#include "quakedef.h"
#include "winquake.h"
#ifdef D3D9QUAKE
#if !defined(HMONITOR_DECLARED) && (WINVER < 0x0500)
    #define HMONITOR_DECLARED
    DECLARE_HANDLE(HMONITOR);
#endif
#include <d3d9.h>
extern LPDIRECT3DDEVICE9 pD3DDev9;

void D3D9_DestroyTexture (texid_t tex)
{
	if (!tex)
		return;

	if (tex->ptr)
		IDirect3DTexture9_Release((IDirect3DTexture9*)tex->ptr);
	tex->ptr = NULL;
}

qboolean D3D9_LoadTextureMips(image_t *tex, struct pendingtextureinfo *mips)
{
	qbyte *fte_restrict out, *fte_restrict in;
	int x, y, i;
	D3DLOCKED_RECT lock;
	D3DFORMAT fmt;
	D3DSURFACE_DESC desc;
	IDirect3DTexture9 *dt;
	qboolean swap = false;
	unsigned int pixelsize = 4;
	unsigned int blocksize = 0;

	switch(mips->encoding)
	{
	case PTI_RGB565:
		pixelsize = 2;
		fmt = D3DFMT_R5G6B5;
		break;
	case PTI_RGBA4444://not supported on d3d9
		return false;
	case PTI_ARGB4444:
		pixelsize = 2;
		fmt = D3DFMT_A4R4G4B4;
		break;
	case PTI_RGBA5551://not supported on d3d9
		return false;
	case PTI_ARGB1555:
		pixelsize = 2;
		fmt = D3DFMT_A1R5G5B5;
		break;
	case PTI_RGBA8:
//		fmt = D3DFMT_A8B8G8R8;	/*how do we check 
		fmt = D3DFMT_A8R8G8B8;
		swap = true;
		break;
	case PTI_RGBX8:
//		fmt = D3DFMT_X8B8G8R8;
		fmt = D3DFMT_X8R8G8B8;
		swap = true;
		break;
	case PTI_BGRA8:
		fmt = D3DFMT_A8R8G8B8;
		break;
	case PTI_BGRX8:
		fmt = D3DFMT_X8R8G8B8;
		break;

	//too lazy to support these for now
	case PTI_S3RGB1:
	case PTI_S3RGBA1:	//d3d doesn't distinguish between these
		fmt = D3DFMT_DXT1;
		blocksize = 8;
		break;
	case PTI_S3RGBA3:
		fmt = D3DFMT_DXT3;
		blocksize = 16;
		break;
	case PTI_S3RGBA5:
		fmt = D3DFMT_DXT5;
		blocksize = 16;
		break;

	default:	//no idea
		return false;
	}

	if (!pD3DDev9)
		return false;	//can happen on errors
	if (FAILED(IDirect3DDevice9_CreateTexture(pD3DDev9, mips->mip[0].width, mips->mip[0].height, mips->mipcount, 0, fmt, D3DPOOL_MANAGED, &dt, NULL)))
		return false;

	for (i = 0; i < mips->mipcount; i++)
	{
		IDirect3DTexture9_GetLevelDesc(dt, i, &desc);

		if (mips->mip[i].height != desc.Height || mips->mip[i].width != desc.Width)
		{
			IDirect3DTexture9_Release(dt);
			return false;
		}

		IDirect3DTexture9_LockRect(dt, i, &lock, NULL, D3DLOCK_NOSYSLOCK|D3DLOCK_DISCARD);
		//can't do it in one go. pitch might contain padding or be upside down.
		if (blocksize)
		{
			if (lock.Pitch == ((mips->mip[i].width+3)/4)*blocksize)
			//for (y = 0, out = lock.pBits, in = mips->mip[i].data; y < mips->mip[i].height; y++, out += lock.Pitch, in += mips->mip[i].width*pixelsize)
			memcpy(lock.pBits, mips->mip[i].data, mips->mip[i].datasize);
		}
		else if (swap)
		{
			for (y = 0, out = lock.pBits, in = mips->mip[i].data; y < mips->mip[i].height; y++, out += lock.Pitch, in += mips->mip[i].width*4)
			{
				for (x = 0; x < mips->mip[i].width*4; x+=4)
				{
					out[x+0] = in[x+2];
					out[x+1] = in[x+1];
					out[x+2] = in[x+0];
					out[x+3] = in[x+3];
				}
			}
		}
		else
		{
			for (y = 0, out = lock.pBits, in = mips->mip[i].data; y < mips->mip[i].height; y++, out += lock.Pitch, in += mips->mip[i].width*pixelsize)
				memcpy(out, in, mips->mip[i].width*pixelsize);
		}
		IDirect3DTexture9_UnlockRect(dt, i);
	}

	D3D9_DestroyTexture(tex);
	tex->ptr = dt;

	return true;
}
void D3D9_UploadLightmap(lightmapinfo_t *lm)
{
}

#endif
