#include "quakedef.h"
#include "glquake.h"
#include "shader.h"

#ifdef RGLQUAKE

#define MAX_TEXTURE_UNITS 8

typedef struct {
	GLenum currenttextures[MAX_TEXTURE_UNITS];
	GLenum texenvmode[MAX_TEXTURE_UNITS];

	int currenttmu;

	qboolean in2d;
} gl_state_t;
gl_state_t gl_state;

extern int		*lightmap_textures;

void GL_SelectTexture (GLenum target) 
{
	gl_state.currenttmu = target - mtexid0;
	if (qglClientActiveTextureARB)
	{
		qglClientActiveTextureARB(target);
		qglActiveTextureARB(target);
	}
	else
		qglSelectTextureSGIS(target);
}

void GL_MBind( GLenum target, int texnum )
{
	GL_SelectTexture( target );

//	if ( gl_state.currenttextures[texnum-mtexid0] == texnum )
//		return;

	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	bindTexFunc (GL_TEXTURE_2D, texnum);
}

void GL_Bind (int texnum)
{
	if (gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;
	gl_state.currenttextures[gl_state.currenttmu] = texnum;

	bindTexFunc (GL_TEXTURE_2D, texnum);
}
void GL_BindType (int type, int texnum)
{
	bindTexFunc (type, texnum);

	gl_state.currenttextures[gl_state.currenttmu] = -1;
}

//vid restarted.
void GL_FlushBinds(void)
{
	int i;
	for (i = 0; i < MAX_TEXTURE_UNITS; i++)
	{
		gl_state.currenttextures[i] = -1;
		gl_state.texenvmode[i] = -1;
	}
}







#ifndef Q3SHADERS


#define MAX_MESH_VERTS 8192

//we don't support multitexturing yet.


static float tempstarray[MAX_MESH_VERTS*3];
static vec4_t tempxyzarray[MAX_MESH_VERTS];

shader_t nullshader, wallbumpshader, modelbumpshader;
#define frand() (rand()&32767)* (1.0/32767)

#define FTABLE_SIZE		1024
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table ? table[FTABLE_CLAMP(x)] : frand())//*((x)-floor(x)))

static	float	r_sintable[FTABLE_SIZE];
static	float	r_triangletable[FTABLE_SIZE];
static	float	r_squaretable[FTABLE_SIZE];
static	float	r_sawtoothtable[FTABLE_SIZE];
static	float	r_inversesawtoothtable[FTABLE_SIZE];

void R_BackendInit(void)
{
	int i;
	double t;
	for ( i = 0; i < FTABLE_SIZE; i++ )
	{
		t = (double)i / (double)FTABLE_SIZE;

		r_sintable[i] = sin ( t * M_PI*2 );
		
		if (t < 0.25) 
			r_triangletable[i] = t * 4.0;
		else if (t < 0.75)
			r_triangletable[i] = 2 - 4.0 * t;
		else
			r_triangletable[i] = (t - 0.75) * 4.0 - 1.0;

		if (t < 0.5) 
			r_squaretable[i] = 1.0f;
		else
			r_squaretable[i] = -1.0f;

		r_sawtoothtable[i] = t;
		r_inversesawtoothtable[i] = 1.0 - t;
	}

	{
		nullshader.numdeforms = 0;//1;
		nullshader.deforms[0].type = DEFORMV_WAVE;
		nullshader.deforms[0].args[0] = 10;
		nullshader.deforms[0].func.type = SHADER_FUNC_SIN;
		nullshader.deforms[0].func.args[1] = 1;
		nullshader.deforms[0].func.args[3] = 10;

		nullshader.passes[0].texturetype = GL_TEXTURE_2D;
		nullshader.passes[0].envmode = GL_MODULATE;
		nullshader.passes[0].blendsrc = GL_SRC_ALPHA;
		nullshader.passes[0].blenddst = GL_ONE_MINUS_SRC_ALPHA;

		nullshader.passes[1].flags |= SHADER_PASS_BLEND;
		nullshader.passes[1].tcgen = TC_GEN_LIGHTMAP;
		nullshader.passes[1].blendsrc = GL_SRC_ALPHA;
		nullshader.passes[1].blenddst = GL_ONE_MINUS_SRC_ALPHA;
		nullshader.passes[1].texturetype = GL_TEXTURE_2D;
	}

	{
		modelbumpshader.numpasses = 3;

		if (1)
			modelbumpshader.passes[0].numMergedPasses = 4;
		else
			modelbumpshader.passes[0].numMergedPasses = 2;
		modelbumpshader.passes[2].numMergedPasses = 1;

		modelbumpshader.passes[0].tcgen = TC_GEN_BASE;
		modelbumpshader.passes[0].envmode = GL_COMBINE_ARB;
		modelbumpshader.passes[0].combinesrc0 = GL_TEXTURE;
		modelbumpshader.passes[0].combinemode = GL_REPLACE;
		modelbumpshader.passes[0].blendsrc = GL_SRC_ALPHA;
		modelbumpshader.passes[0].blenddst = GL_ONE_MINUS_SRC_ALPHA;
		modelbumpshader.passes[0].anim_frames[0] = 0;//bumpmap
		modelbumpshader.passes[0].texturetype = GL_TEXTURE_2D;

		modelbumpshader.passes[1].tcgen = TC_GEN_DOTPRODUCT;
		modelbumpshader.passes[1].envmode = GL_COMBINE_ARB;
		modelbumpshader.passes[1].combinesrc0 = GL_TEXTURE;
		modelbumpshader.passes[1].combinesrc1 = GL_PREVIOUS_ARB;
		modelbumpshader.passes[1].combinemode = GL_DOT3_RGB_ARB;
		modelbumpshader.passes[1].anim_frames[0] = 0;//delux
		modelbumpshader.passes[1].texturetype = GL_TEXTURE_2D;

		modelbumpshader.passes[2].flags |= SHADER_PASS_BLEND;
		modelbumpshader.passes[2].tcgen = TC_GEN_BASE;
		modelbumpshader.passes[2].envmode = GL_MODULATE;
		modelbumpshader.passes[2].blendsrc = GL_DST_COLOR;
		modelbumpshader.passes[2].blenddst = GL_ZERO;
		modelbumpshader.passes[2].anim_frames[0] = 0;//texture
		modelbumpshader.passes[2].texturetype = GL_TEXTURE_2D;

		//gl_combine states that we need to use a textures.
		modelbumpshader.passes[3].tcgen = TC_GEN_BASE;	//multiply by colors
		modelbumpshader.passes[3].envmode = GL_COMBINE_ARB;
		modelbumpshader.passes[3].combinesrc0 = GL_PREVIOUS_ARB;
		modelbumpshader.passes[3].combinesrc1 = GL_PRIMARY_COLOR_ARB;
		modelbumpshader.passes[3].combinemode = GL_MODULATE;
		modelbumpshader.passes[3].anim_frames[0] = 1;		//any, has to be present
		modelbumpshader.passes[3].texturetype = GL_TEXTURE_2D;
	}
	
	{
		wallbumpshader.numpasses = 4;

		if (1)
			wallbumpshader.passes[0].numMergedPasses = 4;
		else
			wallbumpshader.passes[0].numMergedPasses = 2;
		wallbumpshader.passes[2].numMergedPasses = 2;

		wallbumpshader.passes[0].tcgen = TC_GEN_BASE;
		wallbumpshader.passes[0].envmode = GL_COMBINE_ARB;
		wallbumpshader.passes[0].combinesrc0 = GL_TEXTURE;
		wallbumpshader.passes[0].combinemode = GL_REPLACE;
		wallbumpshader.passes[0].anim_frames[0] = 0;//bumpmap
		wallbumpshader.passes[0].blendsrc = GL_SRC_ALPHA;
		wallbumpshader.passes[0].blenddst = GL_ONE_MINUS_SRC_ALPHA;
		wallbumpshader.passes[0].texturetype = GL_TEXTURE_2D;

		wallbumpshader.passes[1].tcgen = TC_GEN_LIGHTMAP;
		wallbumpshader.passes[1].envmode = GL_COMBINE_ARB;
		wallbumpshader.passes[1].combinesrc0 = GL_TEXTURE;
		wallbumpshader.passes[1].combinesrc1 = GL_PREVIOUS_ARB;
		wallbumpshader.passes[1].combinemode = GL_DOT3_RGB_ARB;
		wallbumpshader.passes[1].anim_frames[0] = 0;//delux
		wallbumpshader.passes[1].texturetype = GL_TEXTURE_2D;

		wallbumpshader.passes[2].flags |= SHADER_PASS_BLEND;
		wallbumpshader.passes[2].tcgen = TC_GEN_BASE;
		wallbumpshader.passes[2].envmode = GL_MODULATE;
		wallbumpshader.passes[2].blendsrc = GL_DST_COLOR;
		wallbumpshader.passes[2].blenddst = GL_ZERO;
		wallbumpshader.passes[2].anim_frames[0] = 0;//texture
		wallbumpshader.passes[2].texturetype = GL_TEXTURE_2D;

		wallbumpshader.passes[3].tcgen = TC_GEN_LIGHTMAP;
		wallbumpshader.passes[3].envmode = GL_BLEND;
		wallbumpshader.passes[3].anim_frames[0] = 0;//lightmap
		wallbumpshader.passes[3].texturetype = GL_TEXTURE_2D;
	}
}

static float *R_TableForFunc ( unsigned int func )
{
	switch (func)
	{
	case SHADER_FUNC_SIN:
			return r_sintable;

		case SHADER_FUNC_TRIANGLE:
			return r_triangletable;

		case SHADER_FUNC_SQUARE:
			return r_squaretable;

		case SHADER_FUNC_SAWTOOTH:
			return r_sawtoothtable;

		case SHADER_FUNC_INVERSESAWTOOTH:
			return r_inversesawtoothtable;
	}

	// assume noise
	return NULL;
}

static void MakeDeforms(shader_t *shader, vec4_t *out, vec4_t *in, int number)
{
	float *table;
	int d, j;
	float args[4], deflect;
	deformv_t *dfrm = shader->deforms; 

	for (d = 0; d < shader->numdeforms; d++, dfrm++)
	{
		switch(dfrm->type)
		{
		case DEFORMV_WAVE:
			args[0] = dfrm->func.args[0];
			args[1] = dfrm->func.args[1];
			args[3] = dfrm->func.args[2] + dfrm->func.args[3] * realtime;
			table = R_TableForFunc ( dfrm->func.type );

			for ( j = 0; j < number; j++ ) {
				deflect = dfrm->args[0] * (in[j][0]+in[j][1]+in[j][2]) + args[3];
				deflect = sin(deflect)/*FTABLE_EVALUATE ( table, deflect )*/ * args[1] + args[0];

				out[j][0] = in[j][0]+deflect;
				out[j][1] = in[j][1]+deflect;
				out[j][2] = in[j][2]+deflect;

				// Deflect vertex along its normal by wave amount
//				VectorMA ( out[j], deflect, normalsArray[j], in[j] );
			}
			break;
		case DEFORMV_MOVE:
			table = R_TableForFunc ( dfrm->func.type );
			deflect = dfrm->func.args[2] + realtime * dfrm->func.args[3];
			deflect = FTABLE_EVALUATE (table, deflect) * dfrm->func.args[1] + dfrm->func.args[0];

			for ( j = 0; j < number; j++ )
				VectorMA ( out[j], deflect, dfrm->args, in[j] );
			break;
		default:
			Sys_Error("Bad deform type %i\n", dfrm->type);
		}

		in = out;
	}
}

static void Mesh_DeformTextureCoords(mesh_t *mesh, shaderpass_t *pass)
{
	int d;
//	tcmod_t *tcmod = pass->tcmod;
	float *in, *out;
	switch(pass->tcgen)
	{
	case TC_GEN_DOTPRODUCT:	//take normals, use the dotproduct and produce texture coords for bumpmapping
		{			
			out = tempstarray;
			in = (float*)mesh->normals_array;

			for (d = 0; d < mesh->numvertexes; d++)
			{
				out[d*3+0] = DotProduct((in+d*3), mesh->lightaxis[0]);
				out[d*3+1] = DotProduct((in+d*3), mesh->lightaxis[1]);
				out[d*3+2] = DotProduct((in+d*3), mesh->lightaxis[2]);
			}
			glTexCoordPointer(3, GL_FLOAT, 0, out);
			
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		return;
	case TC_GEN_LIGHTMAP:
		in = (float*)mesh->lmst_array;
		if (in)
			break;	//fallthrought
	case TC_GEN_BASE:
		in = (float*)mesh->st_array;
		break;
	default:
		Sys_Error("Mesh_DeformTextureCoords: Bad TC_GEN type\n");
		return;
	}
/*
	for (d = 0; d < pass->numtcmods; d++, dfrm++)
	{
	}
*/

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer(2, GL_FLOAT, 0, in);
}

static void Mesh_SetShaderpassState ( shaderpass_t *pass, qboolean mtex )
{
	if ( (mtex && (pass->blendmode != GL_REPLACE)) || (pass->flags & SHADER_PASS_BLEND) )
	{
		glEnable (GL_BLEND);
		glBlendFunc (pass->blendsrc, pass->blenddst);
	}
	else
	{
//		glDisable (GL_BLEND);
	}

	if (pass->flags & SHADER_PASS_ALPHAFUNC)
	{
		glEnable (GL_ALPHA_TEST);

		if (pass->alphafunc == SHADER_ALPHA_GT0)
		{
			glAlphaFunc (GL_GREATER, 0);
		}
		else if (pass->alphafunc == SHADER_ALPHA_LT128)
		{
			glAlphaFunc (GL_LESS, 0.5f);
		}
		else if (pass->alphafunc == SHADER_ALPHA_GE128)
		{
			glAlphaFunc (GL_GEQUAL, 0.5f);
		}
	}
	else
	{
//		glDisable (GL_ALPHA_TEST);
	}

//	glDepthFunc (pass->depthfunc);

	if (pass->flags & SHADER_PASS_DEPTHWRITE)
	{
		glDepthMask (GL_TRUE);
	}
	else
	{
//		glDepthMask (GL_FALSE);
	}
}

static void Mesh_DrawPass(shaderpass_t *pass, mesh_t *mesh)
{
	Mesh_SetShaderpassState(pass, false);
	if (pass->numMergedPasses>1 && gl_mtexarbable)
	{
		int p;
//		Mesh_DrawPass(pass+2,mesh);
//		return;
		for (p = 0; p < pass->numMergedPasses; p++)
		{
			qglActiveTextureARB(GL_TEXTURE0_ARB+p);
			qglClientActiveTextureARB(GL_TEXTURE0_ARB+p);
			GL_BindType(pass[p].texturetype, pass[p].anim_frames[0]);
			glEnable(pass[p].texturetype);

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, pass[p].envmode);
			if (pass[p].envmode == GL_COMBINE_ARB)
			{
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, pass[p].combinesrc0);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, pass[p].combinesrc1);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, pass[p].combinemode);
			}

			Mesh_DeformTextureCoords(mesh, pass+p);
		}
		glDrawElements(GL_TRIANGLES, mesh->numindexes, GL_UNSIGNED_INT, mesh->indexes);
		for (p = pass->numMergedPasses-1; p >= 0; p--)
		{
			qglActiveTextureARB(GL_TEXTURE0_ARB+p);
			qglClientActiveTextureARB(GL_TEXTURE0_ARB+p);
			glDisable(pass[p].texturetype);
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
	}
	else
	{
		Mesh_DeformTextureCoords(mesh, pass);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, pass->envmode);

		GL_Bind(pass->anim_frames[0]);
		if (pass->texturetype != GL_TEXTURE_2D)
		{
			glDisable(pass->texturetype);
			glEnable(pass->texturetype);
		}
		glDrawElements(GL_TRIANGLES, mesh->numindexes, GL_UNSIGNED_INT, mesh->indexes);

		if (pass->texturetype != GL_TEXTURE_2D)
		{
			glDisable(pass->texturetype);
			glEnable(GL_TEXTURE_2D);
		}

		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}

shader_t *currentshader;
void GL_PushShader(shader_t *shader)
{
	if (!shader)
	{
		glDisableClientState( GL_VERTEX_ARRAY );
		return;
	}

	glEnableClientState( GL_VERTEX_ARRAY );
}
void GL_PushMesh(mesh_t *mesh, int deluxnum, int lmnum)
{
}

void GL_DrawMesh(mesh_t *mesh, shader_t *shader, int texturenum, int lmtexturenum)
{
	int i;
	if (!shader)
	{
		shader = &nullshader;
		shader->passes[0].anim_frames[0] = texturenum;
		shader->passes[0].numMergedPasses=1;

		if (lmtexturenum && !texturenum)
		{
			shader->passes[0].anim_frames[0] = lmtexturenum;
			shader->numpasses = 1;
			shader->passes[0].flags |= SHADER_PASS_BLEND;
		}
		else if (lmtexturenum)
		{
			shader->numpasses = 2;
			shader->passes[1].anim_frames[0] = lmtexturenum;//lmtexture;
			shader->passes[0].flags &= ~SHADER_PASS_BLEND;
		}
		else
		{
			shader->passes[0].flags &= ~SHADER_PASS_BLEND;
			shader->numpasses = 1;
		}

		shader->passes[0].texturetype = GL_TEXTURE_2D;
	}
	if (!shader->numdeforms)
		glVertexPointer(3, GL_FLOAT, 16, mesh->xyz_array);
	else
	{
		MakeDeforms(shader, tempxyzarray, mesh->xyz_array, mesh->numvertexes);
		glVertexPointer(3, GL_FLOAT, 16, tempxyzarray);
	}
	if (mesh->normals_array && glNormalPointer)
	{
		glNormalPointer(GL_FLOAT, 0, mesh->normals_array);
		glEnableClientState( GL_NORMAL_ARRAY );
	}

	glEnableClientState( GL_VERTEX_ARRAY );
	if (mesh->colors_array && glColorPointer)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, mesh->colors_array);
		glEnableClientState( GL_COLOR_ARRAY );
	}

	for (i =0 ; i < shader->numpasses; i+=shader->passes[i].numMergedPasses)
		Mesh_DrawPass(shader->passes+i, mesh);

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );

/*	//show normals
	if (mesh->normals_array)
	{
		glColor3f(1,1,1);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		for (i = 0; i < mesh->numvertexes; i++)
		{
			glVertex3f(	mesh->xyz_array[i][0],
						mesh->xyz_array[i][1],
						mesh->xyz_array[i][2]);

			glVertex3f(	mesh->xyz_array[i][0] + mesh->normals_array[i][0],
						mesh->xyz_array[i][1] + mesh->normals_array[i][1],
						mesh->xyz_array[i][2] + mesh->normals_array[i][2]);
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
	}
*/
}

void GL_DrawMeshBump(mesh_t *mesh, int texturenum, int lmtexturenum, int bumpnum, int deluxnum)
{
	shader_t *shader;
	extern int normalisationCubeMap;

	if (lmtexturenum)
	{
		shader = &wallbumpshader;
		shader->passes[3].anim_frames[0] = lmtexturenum;
	}
	else
		shader = &modelbumpshader;

	shader->passes[0].anim_frames[0] = bumpnum;
	if (deluxnum)
	{
		shader->passes[1].anim_frames[0] = deluxnum;
		shader->passes[1].tcgen = TC_GEN_LIGHTMAP;
		shader->passes[1].texturetype = GL_TEXTURE_2D;
	}
	else
	{
		shader->passes[1].anim_frames[0] = normalisationCubeMap;
		shader->passes[1].tcgen = TC_GEN_DOTPRODUCT;
		shader->passes[1].texturetype = GL_TEXTURE_CUBE_MAP_ARB;
	}
	shader->passes[2].anim_frames[0] = texturenum;

//	mesh->colors_array=NULL;	//don't bother coloring it.

	GL_DrawMesh(mesh, shader, 0, 0);
}
























































































































































#else

/*
Copyright (C) 2002-2003 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#define MAX_ARRAY_VERTS 8192
#define MAX_ARRAY_INDEXES 8192
#define MAX_ARRAY_NEIGHBORS 8192
#define MAX_ARRAY_TRIANGLES (8192/3)
#define M_TWO_PI (M_PI*2)

cvar_t r_detailtextures = {"r_detailtextures", "1"};
cvar_t r_showtris = {"r_showtris", "1"};
cvar_t r_shownormals = {"r_shownormals", "1"};

float Q_rsqrt( float number )
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;						// evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

void VectorNormalizeFast( vec3_t v )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( v, v ) );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}



void GL_TexEnv( GLenum mode )
{
	static int lastmodes[MAX_TEXTURE_UNITS] = { -1, -1 };

	if ( mode != lastmodes[gl_state.currenttmu] )
	{
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[gl_state.currenttmu] = mode;
	}
}
typedef vec3_t mat3_t[3];
mat3_t axisDefault={{1, 0, 0},
					{0, 1, 0},
					{0, 0, 1}};

void Matrix3_Transpose (mat3_t in, mat3_t out)
{
	out[0][0] = in[0][0];
	out[1][1] = in[1][1];
	out[2][2] = in[2][2];

	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
}
void Matrix3_Multiply_Vec3 (mat3_t a, vec3_t b, vec3_t product)
{
	product[0] = a[0][0]*b[0] + a[0][1]*b[1] + a[0][2]*b[2];
	product[1] = a[1][0]*b[0] + a[1][1]*b[1] + a[1][2]*b[2];
	product[2] = a[2][0]*b[0] + a[2][1]*b[1] + a[2][2]*b[2];
}

void Matrix3_Multiply (mat3_t in1, mat3_t in2, mat3_t out)
{
	out[0][0] = in1[0][0]*in2[0][0] + in1[0][1]*in2[1][0] + in1[0][2]*in2[2][0];
	out[0][1] = in1[0][0]*in2[0][1] + in1[0][1]*in2[1][1] + in1[0][2]*in2[2][1];
	out[0][2] = in1[0][0]*in2[0][2] + in1[0][1]*in2[1][2] + in1[0][2]*in2[2][2];
	out[1][0] = in1[1][0]*in2[0][0] + in1[1][1]*in2[1][0] +	in1[1][2]*in2[2][0];
	out[1][1] = in1[1][0]*in2[0][1] + in1[1][1]*in2[1][1] + in1[1][2]*in2[2][1];
	out[1][2] = in1[1][0]*in2[0][2] + in1[1][1]*in2[1][2] +	in1[1][2]*in2[2][2];
	out[2][0] = in1[2][0]*in2[0][0] + in1[2][1]*in2[1][0] +	in1[2][2]*in2[2][0];
	out[2][1] = in1[2][0]*in2[0][1] + in1[2][1]*in2[1][1] +	in1[2][2]*in2[2][1];
	out[2][2] = in1[2][0]*in2[0][2] + in1[2][1]*in2[1][2] +	in1[2][2]*in2[2][2];
}

int Matrix3_Compare(mat3_t in, mat3_t out)
{
	return memcmp(in, out, sizeof(mat3_t));
}
extern model_t		*currentmodel;

#define clamp(v,min,max) (v) = (((v)<(min))?(min):(((v)>(max))?(max):(v)))
#define bound(min,v,max) (((v)<(min))?(min):(((v)>(max))?(max):(v)))

extern qbyte FloatToByte( float x );


#define FTABLE_SIZE		1024
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table ? table[FTABLE_CLAMP(x)] : frand()*((x)-floor(x)))

static	float	r_sintable[FTABLE_SIZE];
static	float	r_triangletable[FTABLE_SIZE];
static	float	r_squaretable[FTABLE_SIZE];
static	float	r_sawtoothtable[FTABLE_SIZE];
static	float	r_inversesawtoothtable[FTABLE_SIZE];

index_t			*indexesArray;
int				*neighborsArray;
vec3_t			*trNormalsArray;

vec2_t			*coordsArray;
vec2_t			*lightmapCoordsArray;

vec4_t			vertexArray[MAX_ARRAY_VERTS*2];
vec3_t			normalsArray[MAX_ARRAY_VERTS];

vec4_t			tempVertexArray[MAX_ARRAY_VERTS];
vec3_t			tempNormalsArray[MAX_ARRAY_VERTS];
index_t			tempIndexesArray[MAX_ARRAY_INDEXES];

index_t			inIndexesArray[MAX_ARRAY_INDEXES];
int				inNeighborsArray[MAX_ARRAY_NEIGHBORS];
vec3_t			inTrNormalsArray[MAX_ARRAY_TRIANGLES];
vec2_t			inCoordsArray[MAX_ARRAY_VERTS];
vec2_t			inLightmapCoordsArray[MAX_ARRAY_VERTS];
byte_vec4_t		inColorsArray[MAX_ARRAY_VERTS];

static	vec2_t		tUnitCoordsArray[MAX_TEXTURE_UNITS][MAX_ARRAY_VERTS];
static	byte_vec4_t	colorArray[MAX_ARRAY_VERTS];

int				numVerts, numIndexes, numColors;

qboolean		r_arrays_locked;
qboolean		r_blocked;

int				r_features;

static	int		r_lmtex;

static	int		r_texNums[SHADER_PASS_MAX];
static	int		r_numUnits;

index_t			*currentIndex;
int				*currentTrNeighbor;
float			*currentTrNormal;
float			*currentVertex;
float			*currentNormal;
float			*currentCoords;
float			*currentLightmapCoords;
qbyte			*currentColor;

static	int		r_identityLighting;
static	float	r_localShaderTime;

unsigned int	r_numverts;
unsigned int	r_numtris;
unsigned int	r_numflushes;
int r_backendStart;

void R_ResetTexState (void)
{
	coordsArray = inCoordsArray;
	lightmapCoordsArray = inLightmapCoordsArray;

	currentCoords = coordsArray[0];
	currentLightmapCoords = lightmapCoordsArray[0];

	numColors = 0;
	currentColor = inColorsArray[0];
}


void R_PushIndexes ( index_t *indexes, int *neighbors, vec3_t *trnormals, int numindexes, int features )
{
	int i;
	int numTris;

	// this is a fast path for non-batched geometry, use carefully 
	// used on pics, sprites, .dpm, .md3 and .md2 models
	if ( features & MF_NONBATCHED ) {
		if ( numindexes > MAX_ARRAY_INDEXES ) {
			numindexes = MAX_ARRAY_INDEXES;
		}

		// simply change indexesArray to point at indexes
		numIndexes = numindexes;
		indexesArray = indexes;
		currentIndex = indexesArray + numIndexes;

		if ( neighbors ) {
			neighborsArray = neighbors;
			currentTrNeighbor = neighborsArray + numIndexes;
		}

		if ( trnormals && (features & MF_TRNORMALS) ) {
			numTris = numIndexes / 3;

			trNormalsArray = trnormals;
			currentTrNormal = trNormalsArray[0] + numTris;
		}
	}
	else
	{
		// clamp
		if ( numIndexes + numindexes > MAX_ARRAY_INDEXES ) {
			numindexes = MAX_ARRAY_INDEXES - numIndexes;
		}

		numTris = numindexes / 3;
		numIndexes += numindexes;

		// the following code assumes that R_PushIndexes is fed with triangles...
		for ( i=0; i<numTris; i++, indexes += 3, currentIndex += 3 )
		{
			currentIndex[0] = numVerts + indexes[0];
			currentIndex[1] = numVerts + indexes[1];
			currentIndex[2] = numVerts + indexes[2];

			if ( neighbors ) {
				currentTrNeighbor[0] = numTris + neighbors[0];
				currentTrNeighbor[1] = numTris + neighbors[1];
				currentTrNeighbor[2] = numTris + neighbors[2];

				neighbors += 3;
				currentTrNeighbor += 3;
			}

			if ( trnormals && (features & MF_TRNORMALS) ) {
				VectorCopy ( trnormals[i], currentTrNormal );
				currentTrNormal += 3;
			}
		}
	}
}

void R_PushMesh ( mesh_t *mesh, int features )
{
	int numverts;

	if ( !mesh->indexes || !mesh->xyz_array ) {
		return;
	}

	r_features = features;

	R_PushIndexes ( mesh->indexes, mesh->trneighbors, mesh->trnormals, mesh->numindexes, features );

	numverts = mesh->numvertexes;
	if ( numVerts + numverts > MAX_ARRAY_VERTS ) {
		numverts = MAX_ARRAY_VERTS - numVerts;
	}

	memcpy ( currentVertex, mesh->xyz_array, numverts * sizeof(vec4_t) );
	currentVertex += numverts * 4;

	if ( mesh->normals_array && (features & MF_NORMALS) ) {
		memcpy ( currentNormal, mesh->normals_array, numverts * sizeof(vec3_t) );
		currentNormal += numverts * 3;
	}

	if ( mesh->st_array && (features & MF_STCOORDS) ) {
		if ( features & MF_NONBATCHED ) {
			coordsArray = mesh->st_array;
			currentCoords = coordsArray[0];
		} else {
			memcpy ( currentCoords, mesh->st_array, numverts * sizeof(vec2_t) );
		}

		currentCoords += numverts * 2;
	}

	if ( mesh->lmst_array && (features & MF_LMCOORDS) ) {
		if ( features & MF_NONBATCHED ) {
			lightmapCoordsArray = mesh->lmst_array;
			currentLightmapCoords = lightmapCoordsArray[0];
		} else {
			memcpy ( currentLightmapCoords, mesh->lmst_array, numverts * sizeof(vec2_t) );
		}

		currentLightmapCoords += numverts * 2;
	}

	if ( mesh->colors_array && (features & MF_COLORS) ) {
		memcpy ( currentColor, mesh->colors_array, numverts * sizeof(byte_vec4_t) );
		currentColor += numverts * 4;
	}

	numVerts += numverts;
	r_numverts += numverts;
}



extern index_t			r_quad_indexes[6];// = { 0, 1, 2, 0, 2, 3 };

void R_FinishMeshBuffer ( meshbuffer_t *mb );

static float	frand(void)
{
	return (rand()&32767)* (1.0/32767);
}

//static float	crand(void)
//{
//	return (rand()&32767)* (2.0/32767) - 1;
//}

/*
==============
R_BackendInit
==============
*/
void R_BackendInit (void)
{
	int i;
	double t;

	numVerts = 0;
	numIndexes = 0;
    numColors = 0;

	indexesArray = inIndexesArray;
	currentIndex = indexesArray;
	neighborsArray = inNeighborsArray;
	trNormalsArray = inTrNormalsArray;
	coordsArray = inCoordsArray;
	lightmapCoordsArray = inLightmapCoordsArray;

	currentTrNeighbor = neighborsArray;
	currentTrNormal = trNormalsArray[0];

	currentVertex = vertexArray[0];
	currentNormal = normalsArray[0];

	currentCoords = coordsArray[0];
	currentLightmapCoords = lightmapCoordsArray[0];

	currentColor = inColorsArray[0];

	r_arrays_locked = false;
	r_blocked = false;

	qglVertexPointer( 3, GL_FLOAT, 16, vertexArray );	// padded for SIMD
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

	qglEnableClientState( GL_VERTEX_ARRAY );

	//FIZME: FTE already has some stuff along these lines, surly...
//	if ( !r_ignorehwgamma->value )
//		r_identityLighting = (int)(255.0f / pow(2, max(0, floor(r_overbrightbits->value))));
//	else
		r_identityLighting = 255;

	for ( i = 0; i < FTABLE_SIZE; i++ ) {
		t = (double)i / (double)FTABLE_SIZE;

		r_sintable[i] = sin ( t * M_TWO_PI );
		
		if (t < 0.25) 
			r_triangletable[i] = t * 4.0;
		else if (t < 0.75)
			r_triangletable[i] = 2 - 4.0 * t;
		else
			r_triangletable[i] = (t - 0.75) * 4.0 - 1.0;

		if (t < 0.5) 
			r_squaretable[i] = 1.0f;
		else
			r_squaretable[i] = -1.0f;

		r_sawtoothtable[i] = t;
		r_inversesawtoothtable[i] = 1.0 - t;
	}
}

void R_IBrokeTheArrays(void)
{
	qglVertexPointer( 3, GL_FLOAT, 16, vertexArray );	// padded for SIMD
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

	qglEnableClientState( GL_VERTEX_ARRAY );
}

/*
==============
R_BackendShutdown
==============
*/
void R_BackendShutdown (void)
{
}

/*
==============
R_FastSin
==============
*/
float R_FastSin ( float t )
{
	return r_sintable[FTABLE_CLAMP(t)];
}

/*
==============
R_TableForFunc
==============
*/
static float *R_TableForFunc ( unsigned int func )
{
	switch (func)
	{
		case SHADER_FUNC_SIN:
			return r_sintable;

		case SHADER_FUNC_TRIANGLE:
			return r_triangletable;

		case SHADER_FUNC_SQUARE:
			return r_squaretable;

		case SHADER_FUNC_SAWTOOTH:
			return r_sawtoothtable;

		case SHADER_FUNC_INVERSESAWTOOTH:
			return r_inversesawtoothtable;
	}

	// assume noise
	return NULL;
}

/*
==============
R_BackendStartFrame
==============
*/
void R_BackendStartFrame (void)
{
	r_numverts = 0;
	r_numtris = 0;
	r_numflushes = 0;
//	r_backendStart = Sys_Milliseconds();
}

/*
==============
R_BackendEndFrame
==============
*/
void R_BackendEndFrame (void)
{
	if (r_speeds.value)
	{
		Con_Printf( "%4i wpoly %4i leafs %4i verts %4i tris %4i flushes\n",
			c_brush_polys, 
			0/*c_world_leafs*/,
			r_numverts,
			r_numtris,
			r_numflushes ); 
	}
//	time_backend = Sys_Milliseconds() - r_backendStart;
//	r_backendStart = 0;
}

/*
==============
R_LockArrays
==============
*/
void R_LockArrays ( int numverts )
{
	if ( r_arrays_locked )
		return;

	if ( qglLockArraysEXT != 0 ) {
		qglLockArraysEXT( 0, numverts );
		r_arrays_locked = true;
	}
}

/*
==============
R_UnlockArrays
==============
*/
void R_UnlockArrays (void)
{
	if ( !r_arrays_locked )
		return;

	if ( qglUnlockArraysEXT != 0 ) {
		qglUnlockArraysEXT();
		r_arrays_locked = false;
	}
}

/*
==============
R_DrawTriangleStrips

This function looks for and sends tristrips.
Original code by Stephen C. Taylor (Aftershock 3D rendering engine)
==============
*/
void R_DrawTriangleStrips (index_t *indexes, int numindexes)
{
	int toggle;
	index_t a, b, c, *index;

	c = 0;
	index = indexes;
	while ( c < numindexes ) {
		toggle = 1;

		qglBegin( GL_TRIANGLE_STRIP );
		
		qglArrayElement( index[0] );
		qglArrayElement( b = index[1] );
		qglArrayElement( a = index[2] );

		c += 3;
		index += 3;

		while ( c < numindexes ) {
			if ( a != index[0] || b != index[1] ) {
				break;
			}

			if ( toggle ) {
				qglArrayElement( b = index[2] );
			} else {
				qglArrayElement( a = index[2] );
			}

			c += 3;
			index += 3;
			toggle = !toggle;
		}

		qglEnd();
    }
}

/*
==============
R_ClearArrays
==============
*/
void R_ClearArrays (void)
{
	numVerts = 0;
	numIndexes = 0;

	indexesArray = inIndexesArray;
	currentIndex = indexesArray;
	neighborsArray = inNeighborsArray;
	trNormalsArray = inTrNormalsArray;

	currentTrNeighbor = neighborsArray;
	currentTrNormal = trNormalsArray[0];

	currentVertex = vertexArray[0];
	currentNormal = normalsArray[0];

	R_ResetTexState ();

	r_blocked = false;
}

/*
==============
R_FlushArrays
==============
*/
void R_FlushArrays (void)
{
	if ( !numVerts || !numIndexes ) {
		return;
	}

	if ( numColors > 1 ) {
		qglEnableClientState( GL_COLOR_ARRAY );
	} else if ( numColors == 1 ) {
		qglColor4ubv ( colorArray[0] );
	}

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( !r_arrays_locked ) {
		R_DrawTriangleStrips ( indexesArray, numIndexes );
	} else {
		qglDrawElements( GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT,	indexesArray );
	}

	r_numtris += numIndexes / 3;

	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( numColors > 1 ) {
		qglDisableClientState( GL_COLOR_ARRAY );
	}

	r_numflushes++;
}

/*
==============
R_FlushArraysMtex
==============
*/
void R_FlushArraysMtex (void)
{
	int i;

	if ( !numVerts || !numIndexes ) {
		return;
	}

	GL_MBind( mtexid0, r_texNums[0] );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( numColors > 1 ) {
		qglEnableClientState( GL_COLOR_ARRAY );
	} else if ( numColors == 1 ) {
		qglColor4ubv ( colorArray[0] );
	}

	for ( i = 1; i < r_numUnits; i++ )
	{
		GL_MBind( mtexid0 + i, r_texNums[i] );
		qglEnable ( GL_TEXTURE_2D );
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	if ( !r_arrays_locked ) {
		R_DrawTriangleStrips ( indexesArray, numIndexes );
	} else {
		qglDrawElements( GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT,	indexesArray );
	}

	r_numtris += numIndexes / 3;

	for ( i = r_numUnits - 1; i >= 0; i-- )
	{
		GL_SelectTexture ( mtexid0 + i );

		if ( i ) {
			qglDisable ( GL_TEXTURE_2D );
		}
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	if ( numColors > 1 ) {
		qglDisableClientState( GL_COLOR_ARRAY );
	}

	r_numflushes++;
}

/*
================
R_DeformVertices
================
*/
void R_DeformVertices ( meshbuffer_t *mb )
{
	int i, j, k, pw, ph, p;
	float args[4], deflect;
	float *quad[4], *table;
	shader_t *shader;
	deformv_t *deformv;
	vec3_t tv, rot_centre;

	shader = mb->shader;
	deformv = &shader->deforms[0];

	for (i = 0; i < shader->numdeforms; i++, deformv++)
	{
		switch (deformv->type)
		{
			case DEFORMV_NONE:
				break;

			case DEFORMV_WAVE:
				args[0] = deformv->func.args[0];
				args[1] = deformv->func.args[1];
				args[3] = deformv->func.args[2] + deformv->func.args[3] * r_localShaderTime;
				table = R_TableForFunc ( deformv->func.type );

				for ( j = 0; j < numVerts; j++ ) {
					deflect = deformv->args[0] * (vertexArray[j][0]+vertexArray[j][1]+vertexArray[j][2]) + args[3];
					deflect = FTABLE_EVALUATE ( table, deflect ) * args[1] + args[0];

					// Deflect vertex along its normal by wave amount
					VectorMA ( vertexArray[j], deflect, normalsArray[j], vertexArray[j] );
				}
				break;

			case DEFORMV_NORMAL:
				args[0] = deformv->args[1] * r_localShaderTime;

				for ( j = 0; j < numVerts; j++ ) {
					args[1] = normalsArray[j][2] * args[0];

					deflect = deformv->args[0] * R_FastSin ( args[1] );
					normalsArray[j][0] *= deflect;
					deflect = deformv->args[0] * R_FastSin ( args[1] + 0.25 );
					normalsArray[j][1] *= deflect;
					VectorNormalizeFast ( normalsArray[j] );
				}
				break;

			case DEFORMV_MOVE:
				table = R_TableForFunc ( deformv->func.type );
				deflect = deformv->func.args[2] + r_localShaderTime * deformv->func.args[3];
				deflect = FTABLE_EVALUATE (table, deflect) * deformv->func.args[1] + deformv->func.args[0];

				for ( j = 0; j < numVerts; j++ )
					VectorMA ( vertexArray[j], deflect, deformv->args, vertexArray[j] );
				break;

			case DEFORMV_BULGE:
				pw = mb->mesh->patchWidth;
				ph = mb->mesh->patchHeight;

				args[0] = deformv->args[0] / (float)ph;
				args[1] = deformv->args[1];
				args[2] = r_localShaderTime / (deformv->args[2]*pw);

				for ( k = 0, p = 0; k < ph; k++ ) {
					deflect = R_FastSin ( (float)k * args[0] + args[2] ) * args[1];

					for ( j = 0; j < pw; j++, p++ )
						VectorMA ( vertexArray[p], deflect, normalsArray[p], vertexArray[p] );
				}
				break;

			case DEFORMV_AUTOSPRITE:
				if ( numIndexes < 6 )
					break;

				for ( k = 0; k < numIndexes; k += 6 )
				{
					mat3_t m0, m1, result;

					quad[0] = (float *)(vertexArray + indexesArray[k+0]);
					quad[1] = (float *)(vertexArray + indexesArray[k+1]);
					quad[2] = (float *)(vertexArray + indexesArray[k+2]);

					for ( j = 2; j >= 0; j-- ) {
						quad[3] = (float *)(vertexArray + indexesArray[k+3+j]);
						if ( !VectorCompare (quad[3], quad[0]) && 
							!VectorCompare (quad[3], quad[1]) &&
							!VectorCompare (quad[3], quad[2]) ) {
							break;
						}
					}

					VectorSubtract ( quad[0], quad[1], m0[0] );
					VectorSubtract ( quad[2], quad[1], m0[1] );
					CrossProduct ( m0[0], m0[1], m0[2] );
					VectorNormalizeFast ( m0[2] );
					VectorVectors ( m0[2], m0[1], m0[0] );

					VectorCopy ( (&r_world_matrix[0]), m1[0] );
					VectorCopy ( (&r_world_matrix[4]), m1[1] );
					VectorCopy ( (&r_world_matrix[8]), m1[2] );

					Matrix3_Multiply ( m1, m0, result );

					for ( j = 0; j < 3; j++ )
						rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25 + currententity->origin[j];

					for ( j = 0; j < 4; j++ ) {
						VectorSubtract ( quad[j], rot_centre, tv );
						Matrix3_Multiply_Vec3 ( result, tv, quad[j] );
						VectorAdd ( rot_centre, quad[j], quad[j] );
					}
				}
				break;

			case DEFORMV_AUTOSPRITE2:
				if ( numIndexes < 6 )
					break;

				for ( k = 0; k < numIndexes; k += 6 )
				{
					int long_axis, short_axis;
					vec3_t axis;
					float len[3];
					mat3_t m0, m1, m2, result;

					quad[0] = (float *)(vertexArray + indexesArray[k+0]);
					quad[1] = (float *)(vertexArray + indexesArray[k+1]);
					quad[2] = (float *)(vertexArray + indexesArray[k+2]);

					for ( j = 2; j >= 0; j-- ) {
						quad[3] = (float *)(vertexArray + indexesArray[k+3+j]);
						if ( !VectorCompare (quad[3], quad[0]) && 
							!VectorCompare (quad[3], quad[1]) &&
							!VectorCompare (quad[3], quad[2]) ) {
							break;
						}
					}

					// build a matrix were the longest axis of the billboard is the Y-Axis
					VectorSubtract ( quad[1], quad[0], m0[0] );
					VectorSubtract ( quad[2], quad[0], m0[1] );
					VectorSubtract ( quad[2], quad[1], m0[2] );
					len[0] = DotProduct ( m0[0], m0[0] );
					len[1] = DotProduct ( m0[1], m0[1] );
					len[2] = DotProduct ( m0[2], m0[2] );

					if ( (len[2] > len[1]) && (len[2] > len[0]) )
					{
						if ( len[1] > len[0] )
						{
							long_axis = 1;
							short_axis = 0;
						}
						else
						{
							long_axis = 0;
							short_axis = 1;
						}
					}
					else if ( (len[1] > len[2]) && (len[1] > len[0]) )
					{
						if ( len[2] > len[0] )
						{
							long_axis = 2;
							short_axis = 0;
						}
						else
						{
							long_axis = 0;
							short_axis = 2;
						}
					}
					else //if ( (len[0] > len[1]) && (len[0] > len[2]) )
					{
						if ( len[2] > len[1] )
						{
							long_axis = 2;
							short_axis = 1;
						}
						else
						{
							long_axis = 1;
							short_axis = 2;
						}
					}

					if ( DotProduct (m0[long_axis], m0[short_axis]) ) {
						VectorNormalize2 ( m0[long_axis], axis );
						VectorCopy ( axis, m0[1] );

						if ( axis[0] || axis[1] ) {
							VectorVectors ( m0[1], m0[2], m0[0] );
						} else {
							VectorVectors ( m0[1], m0[0], m0[2] );
						}
					} else {
						VectorNormalize2 ( m0[long_axis], axis );
						VectorNormalize2 ( m0[short_axis], m0[0] );
						VectorCopy ( axis, m0[1] );
						CrossProduct ( m0[0], m0[1], m0[2] );
					}

					for ( j = 0; j < 3; j++ )
						rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

					if ( currententity ) {
						VectorAdd ( currententity->origin, rot_centre, tv );
					} else {
						VectorCopy ( rot_centre, tv );
					}
					VectorSubtract ( r_origin, tv, tv );

					// filter any longest-axis-parts off the camera-direction
					deflect = -DotProduct ( tv, axis );

					VectorMA ( tv, deflect, axis, m1[2] );
					VectorNormalizeFast ( m1[2] );
					VectorCopy ( axis, m1[1] );
					CrossProduct ( m1[1], m1[2], m1[0] );

					Matrix3_Transpose ( m1, m2 );
					Matrix3_Multiply ( m2, m0, result );

					for ( j = 0; j < 4; j++ ) {
						VectorSubtract ( quad[j], rot_centre, tv );
						Matrix3_Multiply_Vec3 ( result, tv, quad[j] );
						VectorAdd ( rot_centre, quad[j], quad[j] );
					}
				}
				break;

			case DEFORMV_PROJECTION_SHADOW:
				break;

			default:
				break;
		}
	}
}

/*
==============
R_VertexTCBase
==============
*/
void R_VertexTCBase ( int tcgen, int unit )
{
	int	i;
	vec3_t t, n;
	float *outCoords;
	vec3_t transform;
	mat3_t inverse_axis;
	mat3_t axis;

	outCoords = tUnitCoordsArray[unit][0];
	qglTexCoordPointer( 2, GL_FLOAT, 0, outCoords );

	if ( tcgen == TC_GEN_BASE )
	{
		memcpy ( outCoords, coordsArray[0], sizeof(float) * 2 * numVerts );
	} else if ( tcgen == TC_GEN_LIGHTMAP )
	{
		memcpy ( outCoords, lightmapCoordsArray[0], sizeof(float) * 2 * numVerts );
	}
	else if ( tcgen == TC_GEN_ENVIRONMENT ) 
	{
		if ( !currentmodel )
		{
			VectorSubtract ( vec3_origin, currententity->origin, transform );
			AngleVectors(currententity->angles, axis[0], axis[1], axis[2]);
			Matrix3_Transpose ( axis, inverse_axis );
		}
		else if ( currentmodel == cl.worldmodel )
		{
			VectorSubtract ( vec3_origin, r_origin, transform );
		}
		else if ( currentmodel->type == mod_brush )
		{
			VectorNegate ( currententity->origin, t );
			VectorSubtract ( t, r_origin, transform );
			AngleVectors(currententity->angles, axis[0], axis[1], axis[2]);
			Matrix3_Transpose ( axis, inverse_axis );
		}
		else
		{
			VectorSubtract ( vec3_origin, currententity->origin, transform );
			AngleVectors(currententity->angles, axis[0], axis[1], axis[2]);
			Matrix3_Transpose ( axis, inverse_axis );
		}

		for ( i = 0; i < numVerts; i++, outCoords += 2 )
		{
			VectorAdd ( vertexArray[i], transform, t );

			// project vector
			if ( currentmodel && (currentmodel == cl.worldmodel) )
			{
				n[0] = normalsArray[i][0];
				n[1] = normalsArray[i][1];
				n[2] = Q_rsqrt ( DotProduct(t,t) );
			}
			else
			{
				n[0] = DotProduct ( normalsArray[i], inverse_axis[0] );
				n[1] = DotProduct ( normalsArray[i], inverse_axis[1] );
				n[2] = Q_rsqrt ( DotProduct(t,t) );
			}

			outCoords[0] = t[0]*n[2] - n[0];
			outCoords[1] = t[1]*n[2] - n[1];
		}
	}
	else if ( tcgen == TC_GEN_VECTOR )
	{
		for ( i = 0; i < numVerts; i++, outCoords += 2 )
		{
			static vec3_t tc_gen_s = { 1.0f, 0.0f, 0.0f };
			static vec3_t tc_gen_t = { 0.0f, 1.0f, 0.0f };
			
			outCoords[0] = DotProduct ( tc_gen_s, vertexArray[i] );
			outCoords[1] = DotProduct ( tc_gen_t, vertexArray[i] );
		}
	}

}

/*
==============
R_ShaderpassTex
==============
*/
int R_ShaderpassTex ( shaderpass_t *pass )
{
	if ( pass->flags & SHADER_PASS_ANIMMAP ) {
		return pass->anim_frames[(int)(pass->anim_fps * r_localShaderTime) % pass->anim_numframes];
	}
	else if ( (pass->flags & SHADER_PASS_LIGHTMAP) && r_lmtex >= 0 )
	{
		return lightmap_textures[r_lmtex];
	}

	return pass->anim_frames[0] ? pass->anim_frames[0] : 0;
}

/*
================
R_ModifyTextureCoords
================
*/
void R_ModifyTextureCoords ( shaderpass_t *pass, int unit )
{
	int i, j;
	float *table;
	float t1, t2, sint, cost;
	float *tcArray;
	tcmod_t	*tcmod;

	r_texNums[unit] = R_ShaderpassTex ( pass );

	// we're smart enough not to copy data and simply switch the pointer
	if ( !pass->numtcmods ) {
		if ( pass->tcgen == TC_GEN_BASE ) {
			qglTexCoordPointer( 2, GL_FLOAT, 0, coordsArray );
		} else if ( pass->tcgen == TC_GEN_LIGHTMAP ) {
			qglTexCoordPointer( 2, GL_FLOAT, 0, lightmapCoordsArray );
		} else {
			R_VertexTCBase ( pass->tcgen, unit );
		}
		return;
	}

	R_VertexTCBase ( pass->tcgen, unit );

	for (i = 0, tcmod = pass->tcmods; i < pass->numtcmods; i++, tcmod++)
	{
		tcArray = tUnitCoordsArray[unit][0];

		switch (tcmod->type)
		{
			case SHADER_TCMOD_ROTATE:
				cost = tcmod->args[0] * r_localShaderTime;
				sint = R_FastSin( cost );
				cost = R_FastSin( cost + 0.25 );

				for ( j = 0; j < numVerts; j++, tcArray += 2 ) {
					t1 = cost * (tcArray[0] - 0.5f) - sint * (tcArray[1] - 0.5f) + 0.5f;
					t2 = cost * (tcArray[1] - 0.5f) + sint * (tcArray[0] - 0.5f) + 0.5f;
					tcArray[0] = t1;
					tcArray[1] = t2;
				}
				break;

			case SHADER_TCMOD_SCALE:
				t1 = tcmod->args[0];
				t2 = tcmod->args[1];

				for ( j = 0; j < numVerts; j++, tcArray += 2 ) {
					tcArray[0] = tcArray[0] * t1;
					tcArray[1] = tcArray[1] * t2;
				}
				break;

			case SHADER_TCMOD_TURB:
				t1 = tcmod->args[2] + r_localShaderTime * tcmod->args[3];
				t2 = tcmod->args[1];

				for ( j = 0; j < numVerts; j++, tcArray += 2 ) {
					tcArray[0] = tcArray[0] + R_FastSin (tcArray[0]*t2+t1) * t2;
					tcArray[1] = tcArray[1] + R_FastSin (tcArray[1]*t2+t1) * t2;
				}
				break;
			
			case SHADER_TCMOD_STRETCH:
				table = R_TableForFunc ( tcmod->args[0] );
				t2 = tcmod->args[3] + r_localShaderTime * tcmod->args[4];
				t1 = FTABLE_EVALUATE ( table, t2 ) * tcmod->args[2] + tcmod->args[1];
				t1 = t1 ? 1.0f / t1 : 1.0f;
				t2 = 0.5f - 0.5f * t1;

				for ( j = 0; j < numVerts; j++, tcArray += 2 ) {
					tcArray[0] = tcArray[0] * t1 + t2;
					tcArray[1] = tcArray[1] * t1 + t2;
				}
				break;
						
			case SHADER_TCMOD_SCROLL:
				t1 = tcmod->args[0] * r_localShaderTime;
				t2 = tcmod->args[1] * r_localShaderTime;

				for ( j = 0; j < numVerts; j++, tcArray += 2 ) {
					tcArray[0] = tcArray[0] + t1;
					tcArray[1] = tcArray[1] + t2;
				}
				break;
					
			case SHADER_TCMOD_TRANSFORM:
				for ( j = 0; j < numVerts; j++, tcArray += 2 ) {
					t1 = tcArray[0];
					t2 = tcArray[1];
					tcArray[0] = t1 * tcmod->args[0] + t2 * tcmod->args[2] + tcmod->args[4];
					tcArray[1] = t2 * tcmod->args[1] + t1 * tcmod->args[3] + tcmod->args[5];
				}
				break;

			default:
				break;
		}
	}
}


#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)
#define VectorScalef(a, b, c) c[0]=a[0]*b;c[1]=a[1]*b;c[2]=a[2]*b

/*
================
R_ModifyColor
================
*/
void R_ModifyColor ( meshbuffer_t *mb, shaderpass_t *pass )
{
	int i, b;
	float *table, c, a;
	vec3_t t, v;
	shader_t *shader;
	qbyte *bArray, *vArray;
	qboolean fogged, noArray;
	shaderfunc_t *rgbgenfunc, *alphagenfunc;

	shader = mb->shader;
	fogged = mb->fog && (shader->sort >= SHADER_SORT_UNDERWATER) &&
		!(pass->flags & SHADER_PASS_DEPTHWRITE) && !shader->fog_dist;
	noArray = (pass->flags & SHADER_PASS_NOCOLORARRAY) && !fogged;
	rgbgenfunc = &pass->rgbgen_func;
	alphagenfunc = &pass->alphagen_func;

	if ( noArray ) {
		numColors = 1;
	} else {
		numColors = numVerts;
	}

	bArray = colorArray[0];
	vArray = inColorsArray[0];

	switch (pass->rgbgen)
	{
		case RGB_GEN_IDENTITY:
			memset ( bArray, 255, sizeof(byte_vec4_t)*numColors );
			break;

		case RGB_GEN_IDENTITY_LIGHTING:
			memset ( bArray, r_identityLighting, sizeof(byte_vec4_t)*numColors );
			break;

		case RGB_GEN_CONST:
			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				bArray[0] = FloatToByte (rgbgenfunc->args[0]);
				bArray[1] = FloatToByte (rgbgenfunc->args[1]);
				bArray[2] = FloatToByte (rgbgenfunc->args[2]);
			}
			break;

		case RGB_GEN_WAVE:
			table = R_TableForFunc ( rgbgenfunc->type );
			c = rgbgenfunc->args[2] + r_localShaderTime * rgbgenfunc->args[3];
			c = FTABLE_EVALUATE ( table, c ) * rgbgenfunc->args[1] + rgbgenfunc->args[0];
			clamp ( c, 0.0f, 1.0f );

			memset ( bArray, FloatToByte (c), sizeof(byte_vec4_t)*numColors );
			break;

		case RGB_GEN_ENTITY:
			Con_Printf("RGB_GEN_ENTITY\n");
			break;
/*			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				*(int *)bArray = *(int *)currententity->shaderRGBA;
			}
			break;
*/
		case RGB_GEN_ONE_MINUS_ENTITY:
			Con_Printf("RGB_GEN_ONE_MINUS_ENTITY\n");
			break;
/*			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				bArray[0] = 255 - currententity->shaderRGBA[0];
				bArray[1] = 255 - currententity->shaderRGBA[1];
				bArray[2] = 255 - currententity->shaderRGBA[2];
			}
			break;
*/
		case RGB_GEN_VERTEX:
		case RGB_GEN_EXACT_VERTEX:
			memcpy ( bArray, vArray, sizeof(byte_vec4_t)*numColors );
			break;

		case RGB_GEN_ONE_MINUS_VERTEX:
			for ( i = 0; i < numColors; i++, bArray += 4, vArray += 4 ) {
				bArray[0] = 255 - vArray[0];
				bArray[1] = 255 - vArray[1];
				bArray[2] = 255 - vArray[2];
			}
			break;

		case RGB_GEN_LIGHTING_DIFFUSE:
			if ( !currententity )
			{
				memset ( bArray, 255, sizeof(byte_vec4_t)*numColors );
			}
			else
			{
				memcpy ( bArray, vArray, sizeof(byte_vec4_t)*numColors );
				/*
				vec3_t dif, amb, dir;
				cl.worldmodel->funcs.LightPointValues(currententity->origin, dif, amb, dir);
				bArray[0] = dif[0]*255;
				bArray[1] = dif[1]*255;
				bArray[2] = dif[2]*255;
				//R_LightForEntity ( currententity, bArray );
				*/
			}
			break;

		default:
			break;
	}

	bArray = colorArray[0];
	vArray = inColorsArray[0];

	switch (pass->alphagen)
	{
		case ALPHA_GEN_IDENTITY:
			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				bArray[3] = 255;
			}
			break;

		case ALPHA_GEN_CONST:
			b = FloatToByte ( alphagenfunc->args[0] );

			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				bArray[3] = b;
			}
			break;

		case ALPHA_GEN_WAVE:
			table = R_TableForFunc ( alphagenfunc->type );
			a = alphagenfunc->args[2] + r_localShaderTime * alphagenfunc->args[3];
			a = FTABLE_EVALUATE ( table, a ) * alphagenfunc->args[1] + alphagenfunc->args[0];
			b = FloatToByte ( bound (0.0f, a, 1.0f) );

			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				bArray[3] = b;
			}
			break;

		case ALPHA_GEN_PORTAL:
			VectorAdd ( vertexArray[0], currententity->origin, v );
			VectorSubtract ( r_origin, v, t );
			a = VectorLength ( t ) * (1.0 / 255.0);
			clamp ( a, 0.0f, 1.0f );
			b = FloatToByte ( a );

			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				bArray[3] = b;
			}
			break;

		case ALPHA_GEN_VERTEX:
			for ( i = 0; i < numColors; i++, bArray += 4, vArray += 4 ) {
				bArray[3] = vArray[3];
			}
			break;

		case ALPHA_GEN_ENTITY:
			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				bArray[3] = currententity->alpha*255;
			}
			break;


		case ALPHA_GEN_SPECULAR:
			{
				mat3_t axis;
				AngleVectors(currententity->angles, axis[0], axis[1], axis[2]);
			VectorSubtract ( r_origin, currententity->origin, t );

			if ( !Matrix3_Compare (axis, axisDefault) ) {
				Matrix3_Multiply_Vec3 ( axis, t, v );
			} else {
				VectorCopy ( t, v );
			}

			for ( i = 0; i < numColors; i++, bArray += 4 ) {
				VectorSubtract ( v, vertexArray[i], t );
				a = DotProduct( t, normalsArray[i] ) * Q_rsqrt ( DotProduct(t,t) );
				a = a * a * a * a * a;
				bArray[3] = FloatToByte ( bound (0.0f, a, 1.0f) );
			}
			}
			break;

	}


	if ( fogged )
	{
		float dist, vdist;
		mplane_t *fogplane;
		vec3_t diff, viewtofog, fog_vpn;

		fogplane = mb->fog->visibleplane;
		dist = PlaneDiff ( r_origin, fogplane );

		if ( shader->flags & SHADER_SKY ) 
		{
			if ( dist > 0 )
				VectorScale( fogplane->normal, -dist, viewtofog );
			else
				VectorClear( viewtofog );
		}
		else
		{
			VectorCopy ( currententity->origin, viewtofog );
		}

		VectorScalef ( vpn, mb->fog->shader->fog_dist, fog_vpn );

		bArray = colorArray[0];
		for ( i = 0; i < numColors; i++, bArray += 4 )
		{
			VectorAdd ( vertexArray[i], viewtofog, diff );

			// camera is inside the fog
			if ( dist < 0 ) {
				VectorSubtract ( diff, r_origin, diff );

				c = DotProduct ( diff, fog_vpn );
				a = (1.0f - bound ( 0, c, 1.0f )) * (1.0 / 255.0);
			} else {
				vdist = PlaneDiff ( diff, fogplane );

				if ( vdist < 0 ) {
					VectorSubtract ( diff, r_origin, diff );

					c = vdist / ( vdist - dist );
					c *= DotProduct ( diff, fog_vpn );
					a = (1.0f - bound ( 0, c, 1.0f )) * (1.0 / 255.0);
				} else {
					a = 1.0 / 255.0;
				}
			}

			if ( pass->blendmode == GL_ADD || 
				((pass->blendsrc == GL_ZERO) && (pass->blenddst == GL_ONE_MINUS_SRC_COLOR)) ) {
				bArray[0] = FloatToByte ( (float)bArray[0]*a );
				bArray[1] = FloatToByte ( (float)bArray[1]*a );
				bArray[2] = FloatToByte ( (float)bArray[2]*a );
			} else {
				bArray[3] = FloatToByte ( (float)bArray[3]*a );
			}
		}
	}
}

/*
================
R_SetShaderState
================
*/
void R_SetShaderState ( shader_t *shader )
{
// Face culling
	if ( !gl_cull.value || (r_features & MF_NOCULL) )
	{
		qglDisable ( GL_CULL_FACE );
	}
	else
	{
		if ( shader->flags & SHADER_CULL_FRONT )
		{
			qglEnable ( GL_CULL_FACE );
			qglCullFace ( GL_FRONT );
		}
		else if ( shader->flags & SHADER_CULL_BACK )
		{
			qglEnable ( GL_CULL_FACE );
			qglCullFace ( GL_BACK );
		}
		else
		{
			qglDisable ( GL_CULL_FACE );
		}
	}

	if ( shader->flags & SHADER_POLYGONOFFSET )
	{
		qglEnable ( GL_POLYGON_OFFSET_FILL );
	}
	else
	{
		qglDisable ( GL_POLYGON_OFFSET_FILL );
	}
}

/*
================
R_SetShaderpassState
================
*/
void R_SetShaderpassState ( shaderpass_t *pass, qboolean mtex )
{
	if ( (mtex && (pass->blendmode != GL_REPLACE)) || (pass->flags & SHADER_PASS_BLEND) )
	{
		qglEnable ( GL_BLEND );
		qglBlendFunc ( pass->blendsrc, pass->blenddst );
	}
	else
	{
		qglDisable ( GL_BLEND );
	}

	if ( pass->flags & SHADER_PASS_ALPHAFUNC )
	{
		qglEnable ( GL_ALPHA_TEST );

		if ( pass->alphafunc == SHADER_ALPHA_GT0 )
		{
			qglAlphaFunc ( GL_GREATER, 0 );
		}
		else if ( pass->alphafunc == SHADER_ALPHA_LT128 )
		{
			qglAlphaFunc ( GL_LESS, 0.5f );
		}
		else if ( pass->alphafunc == SHADER_ALPHA_GE128 )
		{
			qglAlphaFunc ( GL_GEQUAL, 0.5f );
		}
	}
	else
	{
		qglDisable ( GL_ALPHA_TEST );
	}

	// nasty hack!!!
	if ( !gl_state.in2d )
	{
		extern int gldepthfunc;
		if (gldepthfunc == GL_LEQUAL)
			qglDepthFunc ( pass->depthfunc );
		else
		{
			switch(pass->depthfunc)
			{
			case GL_LESS:
				qglDepthFunc ( GL_GREATER );
				break;
			case GL_LEQUAL:
				qglDepthFunc ( GL_GEQUAL );
				break;
			case GL_GREATER:
				qglDepthFunc ( GL_LESS );
				break;
			case GL_GEQUAL:
				qglDepthFunc ( GL_LEQUAL );
				break;

			case GL_NEVER:
			case GL_EQUAL:
			case GL_ALWAYS:
			case GL_NOTEQUAL:
			default:
				qglDepthFunc ( pass->depthfunc );
			}
		}

		if ( pass->flags & SHADER_PASS_DEPTHWRITE )
		{
			qglDepthMask ( GL_TRUE );
		}
		else
		{
			qglDepthMask ( GL_FALSE );
		}
	}
}

/*
================
R_RenderMeshGeneric
================
*/
void R_RenderMeshGeneric ( meshbuffer_t *mb, shaderpass_t *pass )
{
	R_SetShaderpassState ( pass, false );
	R_ModifyTextureCoords ( pass, 0 );
	R_ModifyColor ( mb, pass );

	if ( pass->blendmode == GL_REPLACE )
		GL_TexEnv( GL_REPLACE );
	else
		GL_TexEnv ( GL_MODULATE );
	GL_Bind ( r_texNums[0] );

	R_FlushArrays ();
}

/*
================
R_RenderMeshMultitextured
================
*/

void R_RenderMeshMultitextured ( meshbuffer_t *mb, shaderpass_t *pass )
{
	int	i;

	r_numUnits = pass->numMergedPasses;

	R_SetShaderpassState ( pass, true );
	R_ModifyColor ( mb, pass );
	R_ModifyTextureCoords ( pass, 0 );

	GL_SelectTexture( mtexid0 );
	if ( pass->blendmode == GL_REPLACE )
		GL_TexEnv( GL_REPLACE );
	else
		GL_TexEnv( GL_MODULATE );

	for ( i = 1, pass++; i < r_numUnits; i++, pass++ )
	{
		GL_SelectTexture( mtexid0 + i );
		GL_TexEnv( pass->blendmode );
		R_ModifyTextureCoords ( pass, i );
	}

	R_FlushArraysMtex ();
}

/*
================
R_RenderMeshCombined
================
*/
void R_RenderMeshCombined ( meshbuffer_t *mb, shaderpass_t *pass )
{
	int	i;

	r_numUnits = pass->numMergedPasses;

	R_SetShaderpassState ( pass, true );
	R_ModifyColor ( mb, pass );

	GL_SelectTexture( mtexid0 );
	R_ModifyTextureCoords ( pass, 0 );
	if ( pass->blendmode == GL_REPLACE )
		GL_TexEnv( GL_REPLACE );
	else
		GL_TexEnv( GL_MODULATE );

	for ( i = 1, pass++; i < r_numUnits; i++, pass++ )
	{
		GL_SelectTexture( mtexid1 + i );

		if ( pass->blendmode )
		{
			switch ( pass->blendmode )
			{
			case GL_REPLACE:
			case GL_MODULATE:
			case GL_ADD:
				// these modes are best set with TexEnv, Combine4 would need much more setup
				GL_TexEnv (pass->blendmode);
				break;

			case GL_DECAL:
				// mimics Alpha-Blending in upper texture stage, but instead of multiplying the alpha-channel, they�re added
				// this way it can be possible to use GL_DECAL in both texture-units, while still looking good
				// normal mutlitexturing would multiply the alpha-channel which looks ugly
				GL_TexEnv (GL_COMBINE_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_INTERPOLATE_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_ADD);

				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_EXT, GL_SRC_ALPHA);
							
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_SRC_ALPHA);

				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_RGB_EXT, GL_SRC_ALPHA);
				break;
			}
		}
		else
		{
			GL_TexEnv (GL_COMBINE4_NV);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_ADD);

			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_EXT, GL_SRC_ALPHA);

			switch ( pass->blendsrc )
			{
			case GL_ONE:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_ONE_MINUS_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_ZERO:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_SRC_ALPHA);
				break;
			case GL_DST_COLOR:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_DST_COLOR:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_ONE_MINUS_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_SRC_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_ONE_MINUS_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_DST_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_DST_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_ONE_MINUS_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_ONE_MINUS_SRC_ALPHA);
				break;
			}

			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_PREVIOUS_EXT);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_RGB_EXT, GL_SRC_COLOR);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_PREVIOUS_EXT);	
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_EXT, GL_SRC_ALPHA);

			switch (pass->blenddst)
			{
				case GL_ONE:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_ZERO);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_COLOR);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_ZERO);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case GL_ZERO:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_ZERO);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_COLOR);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_ZERO);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
					break;
				case GL_SRC_COLOR:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_COLOR);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
					break;
				case GL_ONE_MINUS_SRC_COLOR:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_COLOR);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case GL_SRC_ALPHA:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_ALPHA);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
					break;
				case GL_ONE_MINUS_SRC_ALPHA:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_ALPHA);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case GL_DST_ALPHA:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_PREVIOUS_EXT);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_ALPHA);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_PREVIOUS_EXT);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
					break;
				case GL_ONE_MINUS_DST_ALPHA:
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_PREVIOUS_EXT);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_ALPHA);
					qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_PREVIOUS_EXT);
					qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
					break;
			}
		}

		R_ModifyTextureCoords ( pass, i );
	}

	R_FlushArraysMtex ();
}

/*
================
R_RenderMeshBuffer
================
*/
void R_RenderMeshBuffer ( meshbuffer_t *mb, qboolean shadowpass )
{
	int	i;
	shader_t *shader;
	shaderpass_t *pass;

	if ( !numVerts ) {
		return;
	}

	shader = mb->shader;
	r_lmtex = mb->infokey;

#ifdef FIZME
	if ( currententity && !gl_state.in2d ) {
		r_localShaderTime = r_refdef.time * 0.001f - currententity->shaderTime;
	} else {
		r_localShaderTime = Sys_Milliseconds() * 0.001f;
	}
#else
	r_localShaderTime = realtime;
#endif

	R_SetShaderState ( shader );

	if ( shader->numdeforms ) {
		R_DeformVertices ( mb );
	}

	if ( !numIndexes || shadowpass ) {
		return;
	}

	R_LockArrays ( numVerts );

	for ( i = 0, pass = shader->passes; i < shader->numpasses; )
	{
		if ( !(pass->flags & SHADER_PASS_DETAIL) || r_detailtextures.value ) {
			pass->flush ( mb, pass );
		}

		i += pass->numMergedPasses;
		pass += pass->numMergedPasses;
	}

	R_FinishMeshBuffer ( mb );
}


/*
================
R_RenderFogOnMesh
================
*/

int r_fogtexture;
#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)
void R_RenderFogOnMesh ( shader_t *shader, struct mfog_s *fog )
{
#define FOG_TEXTURE_HEIGHT 32

	int		i;
	vec3_t	diff, viewtofog, fog_vpn;
	float	dist, vdist;
	shader_t *fogshader;
	mplane_t *fogplane;

	if ( !fog->numplanes || !fog->shader || !fog->visibleplane )
	{
		return;
	}

	R_ResetTexState ();

	fogshader = fog->shader;
	fogplane = fog->visibleplane;

	GL_Bind( r_fogtexture );

	qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	if ( !shader->numpasses || shader->fog_dist || (shader->flags & SHADER_SKY) )
	{
		extern int gldepthfunc;
		qglDepthFunc ( gldepthfunc );
	}
	else
	{
		qglDepthFunc ( GL_EQUAL );
	}

	qglColor4ubv ( fogshader->fog_color );

	// distance to fog
	dist = PlaneDiff ( r_origin, fogplane );

	if ( shader->flags & SHADER_SKY )
	{
		if ( dist > 0 )
			VectorMA( r_origin, -dist, fogplane->normal, viewtofog );
		else
			VectorCopy( r_origin, viewtofog );
	}
	else
	{
		VectorCopy( currententity->origin, viewtofog );
	}

	VectorScale ( vpn, fogshader->fog_dist, fog_vpn );

	for ( i = 0; i < numVerts; i++, currentCoords += 2 )
	{
		VectorAdd ( viewtofog, vertexArray[i], diff );
		vdist = PlaneDiff ( diff, fogplane );
		VectorSubtract ( diff, r_origin, diff );

		if ( dist < 0 )
		{	// camera is inside the fog brush
			currentCoords[0] = DotProduct ( diff, fog_vpn );
		}
		else
		{
			if ( vdist < 0 )
			{
				currentCoords[0] = vdist / ( vdist - dist );
				currentCoords[0] *= DotProduct ( diff, fog_vpn );
			}
			else
			{
				currentCoords[0] = 0.0f;
			}
		}

		currentCoords[1] = -vdist * fogshader->fog_dist + 1.5f/(float)FOG_TEXTURE_HEIGHT;
	}

	if ( !shader->numpasses )
	{
		R_LockArrays ( numVerts );
	}

	R_FlushArrays ();
}

/*
================
R_DrawTriangleOutlines
================
*/
void R_DrawTriangleOutlines (void)
{
	R_ResetTexState ();

	qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_DEPTH_TEST );
	qglColor4f( 1, 1, 1, 1 );
	qglDisable ( GL_BLEND );
	qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

	R_FlushArrays ();

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_TEXTURE_2D );
}

/*
================
R_DrawNormals
================
*/
void R_DrawNormals (void)
{
	int i;

	R_ResetTexState ();

	qglDisable( GL_TEXTURE_2D );
	qglColor4f( 1, 1, 1, 1 );
	qglDisable( GL_BLEND );

	if ( gl_state.in2d ) {
		qglBegin ( GL_POINTS );
		for ( i = 0; i < numVerts; i++ ) { 
			qglVertex3fv ( vertexArray[i] );
		}
		qglEnd ();
	} else {
		qglDisable( GL_DEPTH_TEST );
		qglBegin ( GL_LINES );
		for ( i = 0; i < numVerts; i++ ) { 
			qglVertex3fv ( vertexArray[i] );
			qglVertex3f ( vertexArray[i][0] + normalsArray[i][0], 
				vertexArray[i][1] + normalsArray[i][1], 
				vertexArray[i][2] + normalsArray[i][2] );
		}
		qglEnd ();
		qglEnable( GL_DEPTH_TEST );
	}

	qglEnable( GL_TEXTURE_2D );
}

/*
================
R_FinishMeshBuffer
Render dynamic lights, fog, triangle outlines, normals and clear arrays
================
*/
void R_FinishMeshBuffer ( meshbuffer_t *mb )
{
	shader_t	*shader;
	qboolean	fogged;
	qboolean	dlight;

	shader = mb->shader;
	dlight = (mb->dlightbits != 0) && !(shader->flags & SHADER_FLARE);
	fogged = mb->fog && ((shader->sort < SHADER_SORT_UNDERWATER && 
		(shader->flags & (SHADER_DEPTHWRITE|SHADER_SKY))) || shader->fog_dist);

	if ( dlight || fogged ) {
		GL_DisableMultitexture ( );
		qglTexCoordPointer( 2, GL_FLOAT, 0, inCoordsArray[0] );

		qglEnable ( GL_BLEND );
		qglDisable ( GL_ALPHA_TEST );
		qglDepthMask ( GL_FALSE );

//FIZME
//		if ( dlight ) {
//			R_AddDynamicLights ( mb );
//		}

		if ( fogged ) {
			R_RenderFogOnMesh ( shader, mb->fog );
		}
	}

	if ( r_showtris.value || r_shownormals.value ) {
		GL_DisableMultitexture ( );

		if ( r_showtris.value ) {
			R_DrawTriangleOutlines ();
		}

		if ( r_shownormals.value ) {
			R_DrawNormals ();
		}
	}

	R_UnlockArrays ();
	R_ClearArrays ();
}

#endif







#endif