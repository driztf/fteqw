#if !defined(MINIMAL) && !defined(OMIT_QCC)

#define PROGSUSED
#include "qcc.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <time.h>

#include "errno.h"

//#define DEBUG_DUMP
//#define DISASM "M_Preset"

extern QCC_def_t tempsdef;

char QCC_copyright[1024];
int QCC_packid;
char QCC_Packname[5][128];

extern int optres_test1;
extern int optres_test2;

pbool writeasm;
pbool verbose;
pbool qcc_nopragmaoptimise;
pbool opt_stripunusedfields;
extern unsigned int locals_marshalled;
extern int qccpersisthunk;

pbool QCC_PR_SimpleGetToken (void);
void QCC_PR_LexWhitespace (pbool inhibitpreprocessor);

void *FS_ReadToMem(char *fname, void *membuf, int *len);
void FS_CloseFromMem(void *mem);
const char *QCC_VarAtOffset(QCC_sref_t ref, unsigned int size);

unsigned int MAX_REGS;
unsigned int MAX_LOCALS = 0x10000;
unsigned int MAX_TEMPS = 0x10000;

int	MAX_STRINGS;
int	MAX_GLOBALS;
int	MAX_FIELDS;
int	MAX_STATEMENTS;
int	MAX_FUNCTIONS;
int MAX_CONSTANTS;
int max_temps;

int *qcc_tempofs;
int tempsstart;

#define MAXSOURCEFILESLIST 8
char sourcefileslist[MAXSOURCEFILESLIST][1024];
QCC_def_t *sourcefilesdefs[MAXSOURCEFILESLIST];
int sourcefilesnumdefs;
int currentsourcefile;
int numsourcefiles;
extern char *compilingfile;		//file currently being compiled
char compilingrootfile[1024];	//the .src file we started from (the current one, not original)

char	qccmsourcedir[1024];	//the -src path, for #includes

void QCC_PR_ResetErrorScope(void);

pbool	compressoutput;

pbool newstylesource;
char		destfile[1024];		//the file we're going to output to

QCC_eval_t		*qcc_pr_globals;
unsigned int	numpr_globals;

char		*strings;
int			strofs;

QCC_statement_t	*statements;
int			numstatements;


QCC_function_t	*functions;
//dfunction_t	*dfunctions;
int			numfunctions;

QCC_ddef_t		*qcc_globals;
int			numglobaldefs;

QCC_ddef_t		*fields;
int			numfielddefs;

//typedef char PATHSTRING[MAX_DATA_PATH];

precache_t	*precache_sound;
int			numsounds;

precache_t	*precache_texture;
int			numtextures;

precache_t	*precache_model;
int			nummodels;

precache_t	*precache_file;
int			numfiles;

extern int numCompilerConstants;
hashtable_t compconstantstable;
hashtable_t globalstable;
hashtable_t localstable;
#ifdef WRITEASM
FILE *asmfile;
pbool asmfilebegun;
#endif
hashtable_t floatconstdefstable;
hashtable_t stringconstdefstable;
hashtable_t stringconstdefstable_trans;
extern int dotranslate_count;

unsigned char qccwarningaction[WARN_MAX];	//0 = disabled, 1 = warn, 2 = error.

qcc_targetformat_t qcc_targetformat;

pbool bodylessfuncs;

QCC_type_t *qcc_typeinfo;
int numtypeinfos;
int maxtypeinfos;


struct {
	char *name;
	int index;
} warningnames[] =
{
//	{"", WARN_NOTREFERENCEDCONST},
//	{"", WARN_CONFLICTINGRETURNS},
	{" Q100", WARN_PRECOMPILERMESSAGE},
	{" Q101", WARN_TOOMANYPARAMS},
	//102: Indirect function: too many parameters
	//103: vararg func cannot have more than 
	//104: type mismatch on parm %i
	{" Q105", WARN_TOOFEWPARAMS},
//	{"", WARN_UNEXPECTEDPUNCT},
	{" Q106", WARN_ASSIGNMENTTOCONSTANT},
	//107: Array index should be type int
	//108: Mixed float and int types
	//109: Expecting int, float a parameter found
	//110: Expecting int, float b parameter found
	//112: Null 'if' statement
	//113: Null 'else' statement
	//114: Type mismatch on redeclaration 
	//115: redeclared with different number of parms 
	//116: Local %s redeclared
	//117: too many initializers
	//118: Too many closing braces
	//119: Too many #endifs
	{" Q120", WARN_BADPRAGMA},
	//121: unknown directive
	//122: strofs exceeds limit
	//123: numstatements exceeds limit 
	//124: numfunctions exceeds limit 
	//125: numglobaldefs exceeds limit 
	//126: numfielddefs exceeds limit 
	//127: numpr_globals exceeds limit 
	//128: rededeclared with different parms 
	{" F129", WARN_COMPATIBILITYHACK},	//multiple errors are replaced by this, for compat purposes.

	{" Q203", WARN_MISSINGRETURNVALUE},
	{" Q204", WARN_WRONGRETURNTYPE},
	{" Q205", WARN_POINTLESSSTATEMENT},
	{" Q206", WARN_MISSINGRETURN},
	{" Q207", WARN_DUPLICATEDEFINITION},	//redeclared different scope
	{" Q208", WARN_SYSTEMCRC},
//	{"", WARN_STRINGTOOLONG},
//	{"", WARN_BADTARGET},
	//301: %s defined as local in %s
	{" Q302", WARN_NOTREFERENCED},			//302: Unreferenced local variable %s from line %i
	//401: In function %s parameter %s is unused
//	{"", WARN_HANGINGSLASHR},
//	{"", WARN_NOTDEFINED},
//	{"", WARN_SWITCHTYPEMISMATCH},
//	{"", WARN_CONFLICTINGUNIONMEMBER},
//	{"", WARN_KEYWORDDISABLED},
//	{"", WARN_ENUMFLAGS_NOTINTEGER},
//	{"", WARN_ENUMFLAGS_NOTBINARY},
	{" Q111", WARN_DUPLICATELABEL},
	{" Q201", WARN_ASSIGNMENTINCONDITIONAL},
	{" F300", WARN_DEADCODE},
	{" F301", WARN_NOTUTF8},
	{" F302", WARN_UNINITIALIZED},
	{" F303", WARN_EVILPREPROCESSOR},
	{" F304", WARN_UNARYNOTSCOPE},
	{" F305", WARN_CASEINSENSITIVEFRAMEMACRO},
	{" F306", WARN_SAMENAMEASGLOBAL},
	{" F307", WARN_STRICTTYPEMISMATCH},
	{" F308", WARN_TYPEMISMATCHREDECOPTIONAL},
	{" F309", WARN_IGNORECOMMANDLINE},
	{" F310", WARN_MISUSEDAUTOCVAR},
	{" F311", WARN_FTE_SPECIFIC},
	{" F312", WARN_OVERFLOW},
	{" F313", WARN_DENORMAL},
	{" F314", WARN_LAXCAST},
	{" F315", WARN_DUPLICATEPRECOMPILER},
	{" F316", WARN_IDENTICALPRECOMPILER},

	{" F208", WARN_NOTREFERENCEDCONST},
	{" F209", WARN_EXTRAPRECACHE},	
	{" F210", WARN_NOTPRECACHED},

	//frikqcc errors
	//Q608: PrecacheSound: numsounds
	//Q609: PrecacheModels: nummodels
	//Q610: PrecacheFile: numfiles
	//Q611: Bad parm order
	//Q612: PR_CompileFile: Didn't clear (internal error)
	//Q614: PR_DefForFieldOfs: couldn't find
	//Q613: Error writing error.log
	//Q615: Error writing
	//Q616: No function named
	//Q617: Malloc failure
	//Q618: Ran out of mem pointer space (malloc failure again)
	{NULL}
};

char *QCC_NameForWarning(int idx)
{
	int i;
	for (i = 0; warningnames[i].name; i++)
	{
		if (warningnames[i].index == idx)
			return warningnames[i].name;
	}
	return NULL;
}
int QCC_WarningForName(const char *name)
{
	int i;
	for (i = 0; warningnames[i].name; i++)
	{
		if (!stricmp(name, warningnames[i].name+1))
			return warningnames[i].index;
	}
	return -1;
}

optimisations_t optimisations[] =
{
	//level 0 = no optimisations
	//level 1 = size optimisations
	//level 2 = speed optimisations
	//level 3 = unsafe optimisations (they break multiprogs).
	//level 4 = experimental or extreeme features... must be used explicitly

	{&opt_assignments,				"t",	1,	FLAG_ASDEFAULT,			"assignments",		"c = a*b is performed in one operation rather than two, and can cause older decompilers to fail."},
	{&opt_shortenifnots,			"i",	1,	FLAG_ASDEFAULT,			"shortenifs",		"if (!a) was traditionally compiled in two statements. This optimisation does it in one, but can cause some decompilers to get confused."},
	{&opt_nonvec_parms,				"p",	1,	FLAG_ASDEFAULT,			"nonvec_parms",		"In the original qcc, function parameters were specified as a vector store even for floats. This fixes that."},
	{&opt_constant_names,			"c",	2,	FLAG_KILLSDEBUGGERS,	"constant_names",	"This optimisation strips out the names of constants (but not strings) from your progs, resulting in smaller files. It makes decompilers leave out names or fabricate numerical ones."},
	{&opt_constant_names_strings,	"cs",	3,	FLAG_KILLSDEBUGGERS,	"constant_names_strings", "This optimisation strips out the names of string constants from your progs. However, this can break addons, so don't use it in those cases."},
	{&opt_dupconstdefs,				"d",	1,	FLAG_ASDEFAULT,			"dupconstdefs",		"This will merge definitions of constants which are the same value. Pay extra attention to assignment to constant warnings."},
	{&opt_noduplicatestrings,		"s",	1,	FLAG_ASDEFAULT,			"noduplicatestrings", "This will compact the string table that is stored in the progs. It will be considerably smaller with this."},
	{&opt_locals,					"l",	1,	FLAG_KILLSDEBUGGERS,	"locals",			"Strips out local names and definitions. Most decompiles will break on this."},
	{&opt_function_names,			"n",	1,	FLAG_KILLSDEBUGGERS,	"function_names",	"This strips out the names of functions which are never called. Doesn't make much of an impact though."},
	{&opt_filenames,				"f",	1,	FLAG_KILLSDEBUGGERS,	"filenames",		"This strips out the filenames of the progs. This can confuse the really old decompilers, but is nothing to the more recent ones."},
	{&opt_unreferenced,				"u",	1,	FLAG_ASDEFAULT,			"unreferenced",		"Removes the entries of unreferenced variables. Doesn't make a difference in well maintained code."},
	{&opt_overlaptemps,				"r",	1,	FLAG_ASDEFAULT,			"overlaptemps",		"Optimises the pr_globals count by overlapping temporaries. In QC, every multiplication, division or operation in general produces a temporary variable. This optimisation prevents excess, and in the case of Hexen2's gamecode, reduces the count by 50k. This is the most important optimisation, ever."},
	{&opt_constantarithmatic,		"a",	1,	FLAG_ASDEFAULT,			"constantarithmatic", "5*6 actually emits an operation into the progs. This prevents that happening, effectivly making the compiler see 30"},
	{&opt_precache_file,			"pf",	2,	0,						"precache_file",	"Strip out stuff wasted used in function calls and strings to the precache_file builtin (which is actually a stub in quake)."},
	{&opt_return_only,				"ro",	2,	FLAG_KILLSDEBUGGERS,	"return_only",		"Functions ending in a return statement do not need a done statement at the end of the function. This can confuse some decompilers, making functions appear larger than they were."},
	{&opt_compound_jumps,			"cj",	2,	FLAG_KILLSDEBUGGERS,	"compound_jumps",	"This optimisation plays an effect mostly with nested if/else statements, instead of jumping to an unconditional jump statement, it'll jump to the final destination instead. This will bewilder decompilers."},
//	{&opt_comexprremoval,			"cer",	4,	0,						"expression_removal",	"Eliminate common sub-expressions"},	//this would be too hard...
	{&opt_stripfunctions,			"sf",	3,	FLAG_KILLSDEBUGGERS,	"strip_functions",	"Strips out the 'defs' of functions that were only ever called directly. This does not affect saved games. This can affect FTE_MULTIPROGS."},
	{&opt_locals_overlapping,		"lo",	2,	FLAG_KILLSDEBUGGERS,	"locals_overlapping", "Store all locals in a single section of the pr_globals. Vastly reducing it. This effectivly does the job of overlaptemps.\nHowever, locals are no longer automatically initialised to 0 (and never were in the case of recursion, but at least then its the same type).\nIf locals appear uninitialised, fteqcc will disable this optimisation for the affected functions, you can optionally get a warning about these locals using: #pragma warning enable F302"},
	{&opt_vectorcalls,				"vc",	4,	FLAG_KILLSDEBUGGERS,	"vectorcalls",		"Where a function is called with just a vector, this causes the function call to store three floats instead of one vector. This can save a good number of pr_globals where those vectors contain many duplicate coordinates but do not match entirly."},
	{&opt_classfields,				"cf",	2,	FLAG_KILLSDEBUGGERS,	"class_fields",		"Strip class field names. This will harm debugging and can result in 'gibberish' names appearing in saved games. Has no effect on engines other than FTEQW, which will not recognise these anyway."},
//	{&opt_stripunusedfields,		"uf",	4,	0,						"strip_unused_fields","Strips any fields that have no references. This may result in extra warnings at load time, or disabling support for mapper-specified alpha in engines that do not provide that."},
	{NULL}
};

#define defaultkeyword FLAG_HIDDENINGUI|FLAG_ASDEFAULT|FLAG_MIDCOMPILE
#define nondefaultkeyword FLAG_HIDDENINGUI|0|FLAG_MIDCOMPILE
//global to store useage to, flags, codename, human-readable name, help text
compiler_flag_t compiler_flag[] = {
	//keywords
	{&keyword_asm,			defaultkeyword, "asm",			"Keyword: asm",			"Disables the 'asm' keyword. Use the writeasm flag to see an example of the asm."},
	{&keyword_break,		defaultkeyword, "break",		"Keyword: break",		"Disables the 'break' keyword."},
	{&keyword_case,			defaultkeyword, "case",			"Keyword: case",		"Disables the 'case' keyword."},
	{&keyword_class,		defaultkeyword, "class",		"Keyword: class",		"Disables the 'class' keyword."},
	{&keyword_const,		defaultkeyword, "const",		"Keyword: const",		"Disables the 'const' keyword."},
	{&keyword_continue,		defaultkeyword, "continue",		"Keyword: continue",	"Disables the 'continue' keyword."},
	{&keyword_default,		defaultkeyword, "default",		"Keyword: default",		"Disables the 'default' keyword."},
	{&keyword_entity,		defaultkeyword, "entity",		"Keyword: entity",		"Disables the 'entity' keyword."},
	{&keyword_enum,			defaultkeyword, "enum",			"Keyword: enum",		"Disables the 'enum' keyword."},	//kinda like in c, but typedef not supported.
	{&keyword_enumflags,	defaultkeyword, "enumflags",	"Keyword: enumflags",	"Disables the 'enumflags' keyword."},	//like enum, but doubles instead of adds 1.
	{&keyword_extern,		defaultkeyword, "extern",		"Keyword: extern",		"Disables the 'extern' keyword. Use only on functions inside addons."},	//function is external, don't error or warn if the body was not found
	{&keyword_float,		defaultkeyword, "float",		"Keyword: float",		"Disables the 'float' keyword. (Disables the float keyword without 'local' preceeding it)"},
	{&keyword_for,			defaultkeyword, "for",			"Keyword: for",			"Disables the 'for' keyword. Syntax: for(assignment; while; increment) {codeblock;}"},
	{&keyword_goto,			defaultkeyword, "goto",			"Keyword: goto",		"Disables the 'goto' keyword."},
	{&keyword_int,			defaultkeyword, "int",			"Keyword: int",			"Disables the 'int' keyword."},
	{&keyword_integer,		defaultkeyword, "integer",		"Keyword: integer",		"Disables the 'integer' keyword."},
	{&keyword_noref,		defaultkeyword, "noref",		"Keyword: noref",		"Disables the 'noref' keyword."},	//nowhere else references this, don't warn about it.
	{&keyword_unused,		nondefaultkeyword, "unused",	"Keyword: unused",		"Disables the 'unused' keyword. 'unused' means that the variable is unused, you're aware that its unused, and you'd rather not know about all the warnings this results in."},
	{&keyword_used,			nondefaultkeyword, "used",		"Keyword: used",		"Disables the 'used' keyword. 'used' means that the variable is used even if the qcc can't see how - thus preventing it from ever being stripped."},
	{&keyword_static,		defaultkeyword, "static",		"Keyword: static",		"Disables the 'static' keyword. 'static' means that a variable has altered scope. On globals, the variable is visible only to the current .qc file. On locals, the variable's value does not change between calls to the function. On class variables, specifies that the field is a scoped global instead of a local. On class functions, specifies that 'this' is expected to be invalid and that the function will access any memembers via it."},
	{&keyword_nonstatic,	defaultkeyword, "nonstatic",	"Keyword: nonstatic",	"Disables the 'nonstatic' keyword. 'nonstatic' acts upon globals+functions, reverting the defaultstatic pragma on a per-variable basis. For use by people who prefer to keep their APIs explicit."},
	{&keyword_ignore,		nondefaultkeyword, "ignore",	"Keyword: ignore",		"Disables the 'ignore' keyword. 'ignore' is expected to typically be hidden behind a 'csqconly' define, and in such a context can be used to conditionally compile functions a little more gracefully. The opposite of the 'used' keyword. These variables/functions/members are ALWAYS stripped, and effectively ignored."},

	{&keyword_nosave,		defaultkeyword, "nosave",		"Keyword: nosave",		"Disables the 'nosave' keyword."},	//don't write the def to the output.
	{&keyword_inline,		defaultkeyword, "inline",		"Keyword: inline",		"Disables the 'inline' keyword."},	//don't write the def to the output.
	{&keyword_strip,		defaultkeyword, "strip",		"Keyword: strip",		"Disables the 'strip' keyword."},	//don't write the def to the output.
	{&keyword_shared,		defaultkeyword, "shared",		"Keyword: shared",		"Disables the 'shared' keyword."},	//mark global to be copied over when progs changes (part of FTE_MULTIPROGS)
	{&keyword_state,		nondefaultkeyword,"state",		"Keyword: state",		"Disables the 'state' keyword."},
	{&keyword_optional,		defaultkeyword,"optional",		"Keyword: optional",	"Disables the 'optional' keyword."},
	{&keyword_inout,		nondefaultkeyword,"inout",		"Keyword: inout",		"Disables the 'inout' keyword."},
	{&keyword_string,		defaultkeyword, "string",		"Keyword: string",		"Disables the 'string' keyword."},
	{&keyword_struct,		defaultkeyword, "struct",		"Keyword: struct",		"Disables the 'struct' keyword."},
	{&keyword_switch,		defaultkeyword, "switch",		"Keyword: switch",		"Disables the 'switch' keyword."},
	{&keyword_thinktime,	nondefaultkeyword,"thinktime",	"Keyword: thinktime",	"Disables the 'thinktime' keyword which is used in HexenC"},
	{&keyword_until,		nondefaultkeyword,"until",		"Keyword: until",		"Disables the 'until' keyword which is used in HexenC"},
	{&keyword_loop,			nondefaultkeyword,"loop",		"Keyword: loop",		"Disables the 'loop' keyword which is used in HexenC"},
	{&keyword_typedef,		defaultkeyword, "typedef",		"Keyword: typedef",		"Disables the 'typedef' keyword."},	//fixme
	{&keyword_union,		defaultkeyword, "union",		"Keyword: union",		"Disables the 'union' keyword."},	//you surly know what a union is!
	{&keyword_var,			defaultkeyword, "var",			"Keyword: var",			"Disables the 'var' keyword."},
	{&keyword_vector,		defaultkeyword, "vector",		"Keyword: vector",		"Disables the 'vector' keyword."},
	{&keyword_wrap,			defaultkeyword, "wrap",			"Keyword: wrap",		"Disables the 'wrap' keyword."},
	{&keyword_weak,			defaultkeyword, "weak",			"Keyword: weak",		"Disables the 'weak' keyword."},

	//options
	{&keywords_coexist,		FLAG_ASDEFAULT, "kce",			"Keywords Coexist",		"If you want keywords to NOT be disabled when they a variable by the same name is defined, check here."},
	{&output_parms,			0,				"parms",		"Define offset parms",	"if PARM0 PARM1 etc should be defined by the compiler. These are useful if you make use of the asm keyword for function calls, or you wish to create your own variable arguments. This is an easy way to break decompilers."},	//controls weather to define PARMx for the parms (note - this can screw over some decompilers)
	{&autoprototype,		0,				"autoproto",	"Automatic Prototyping","Causes compilation to take two passes instead of one. The first pass, only the definitions are read. The second pass actually compiles your code. This means you never have to remember to prototype functions again."},	//so you no longer need to prototype functions and things in advance.
	{&writeasm,				0,				"wasm",			"Dump Assembler",		"Writes out a qc.asm which contains all your functions but in assembler. This is a great way to look for bugs in fteqcc, but can also be used to see exactly what your functions turn into, and thus how to optimise statements better."},			//spit out a qc.asm file, containing an assembler dump of the ENTIRE progs. (Doesn't include initialisation of constants)
	{&flag_guiannotate,		FLAG_MIDCOMPILE,"annotate",		"Annotate Sourcecode",	"Annotate source code with assembler statements on compile (requires gui)."},
	{&flag_nullemptystr,	FLAG_MIDCOMPILE,"nullemptystr",	"Null String Immediates",		"Empty string immediates will have the raw value 0 instead of 1."},
	{&flag_ifstring,		FLAG_MIDCOMPILE,"ifstring",		"if(string) fix",		"Causes if(string) to behave identically to if(string!="") This is most useful with addons of course, but also has adverse effects with FRIK_FILE's fgets, where it becomes impossible to determin the end of the file. In such a case, you can still use asm {IF string 2;RETURN} to detect eof and leave the function."},		//correction for if(string) no-ifstring to get the standard behaviour.
	{&flag_iffloat,			FLAG_MIDCOMPILE,"iffloat","if(-0.0) fix","Fixes certain floating point logic."},
	{&flag_ifvector,		FLAG_MIDCOMPILE|FLAG_ASDEFAULT,"ifvector","if('0 1 0') fix","Fixes conditional vector logic."},
	{&flag_vectorlogic,		FLAG_MIDCOMPILE|FLAG_ASDEFAULT,"vectorlogic","v&&v||v fix",	"Fixes conditional vector logic."},
	{&flag_brokenarrays,	FLAG_MIDCOMPILE,"brokenarray",	"array[0] omission",	"Treat references to arrays as references to the first index of said array, to replicate an old fteqcc bug."},
	{&flag_rootconstructor,	FLAG_MIDCOMPILE,"rootconstructor","root constructor first",	"When enabled, the root constructor should be called first like in c++."},
	{&flag_acc,				0,				"acc",			"Reacc support",		"Reacc is a pascall like compiler. It was released before the Quake source was released. This flag has a few effects. It sorts all qc files in the current directory into alphabetical order to compile them. It also allows Reacc global/field distinctions, as well as allows � as EOF. Whilst case insensitivity and lax type checking are supported by reacc, they are seperate compiler flags in fteqcc."},		//reacc like behaviour of src files.
	{&flag_caseinsensitive,	0,				"caseinsens",	"Case insensitivity",	"Causes fteqcc to become case insensitive whilst compiling names. It's generally not advised to use this as it compiles a little more slowly and provides little benefit. However, it is required for full reacc support."},	//symbols will be matched to an insensitive case if the specified case doesn't exist. This should b usable for any mod
	{&flag_laxcasts,		FLAG_MIDCOMPILE,"lax",			"Lax type checks",		"Disables many errors (generating warnings instead) when function calls or operations refer to two normally incompatible types. This is required for reacc support, and can also allow certain (evil) mods to compile that were originally written for frikqcc."},		//Allow lax casting. This'll produce loadsa warnings of course. But allows compilation of certain dodgy code.
	{&flag_hashonly,		FLAG_MIDCOMPILE,"hashonly",		"Hash-only constants",	"Allows use of only #constant for precompiler constants, allows certain preqcc using mods to compile"},
	{&opt_logicops,			FLAG_MIDCOMPILE,"lo",			"Logic ops",			"This changes the behaviour of your code. It generates additional if operations to early-out in if statements. With this flag, the line if (0 && somefunction()) will never call the function. It can thus be considered an optimisation. However, due to the change of behaviour, it is not considered so by fteqcc. Note that due to inprecisions with floats, this flag can cause runaway loop errors within the player walk and run functions (without iffloat also enabled). This code is advised:\nplayer_stand1:\n    if (self.velocity_x || self.velocity_y)\nplayer_run\n    if (!(self.velocity_x || self.velocity_y))"},
	{&flag_msvcstyle,		FLAG_MIDCOMPILE,"msvcstyle",	"MSVC-style errors",	"Generates warning and error messages in a format that msvc understands, to facilitate ide integration."},
	{&flag_debugmacros,		FLAG_MIDCOMPILE,"debugmacros",	"Verbose Macro Expansion",	"Print out the contents of macros that are expanded. This can help look inside macros that are expanded and is especially handy if people are using preprocessor hacks."},
	{&flag_filetimes,		0,				"filetimes",	"Check Filetimes",		"Recompiles the progs only if the file times are modified."},
	{&flag_fasttrackarrays,	FLAG_MIDCOMPILE,"fastarrays",	"fast arrays where possible",	"Generates extra instructions inside array handling functions to detect engine and use extension opcodes only in supporting engines.\nAdds a global which is set by the engine if the engine supports the extra opcodes. Note that this applies to all arrays or none."},
	{&flag_assume_integer,	FLAG_MIDCOMPILE,"assumeint",	"Assume Integers",		"Numerical constants are assumed to be integers, instead of floats."},
	{&pr_subscopedlocals,	FLAG_MIDCOMPILE,"subscope",		"Subscoped Locals",		"Restrict the scope of locals to the block they are actually defined within, as in C."},
	{&verbose,				FLAG_MIDCOMPILE,"verbose",		"Verbose",				"Lots of extra compiler messages."},
	{&flag_typeexplicit,	FLAG_MIDCOMPILE,"typeexplicit",	"Explicit types",		"All type conversions must be explicit or directly supported by instruction set."},
	{&flag_noboundchecks,	FLAG_MIDCOMPILE,"noboundchecks","Disable Bound Checks",	"Disable array index checks, speeding up array access but can result in your code misbehaving."},
	{&flag_qccx,			FLAG_MIDCOMPILE,"qccx",			"QCCX syntax",			"WARNING: This syntax makes mods inherantly engine specific.\nDo NOT use unless you know what you're doing.This is provided for compatibility only\nAny entity hacks will be unsupported in FTEQW, DP, and others, resulting in engine crashes if the code in question is executed."},
	{&flag_embedsrc,		FLAG_MIDCOMPILE,"embedsrc",		"Embed Sources",		"Write the sourcecode into the output file."},
//	{&flag_noreflection,	FLAG_MIDCOMPILE,"omitinternals","Omit Reflection Info",	"Keeps internal symbols private (equivelent to unix's hidden visibility). This has the effect of reducing filesize, thwarting debuggers, and breaking saved games. This allows you to use arrays without massively bloating the size of your progs.\nWARNING: The bit about breaking saved games was NOT a joke, but does not apply to menuqc or csqc. It also interferes with FTE_MULTIPROGS."},
	{NULL}
};

struct {
	qcc_targetformat_t target;
	char *name;
} targets[] = {
	{QCF_STANDARD,	"standard"},
	{QCF_STANDARD,	"q1"},
	{QCF_STANDARD,	"quakec"},
	{QCF_HEXEN2,	"hexen2"},
	{QCF_HEXEN2,	"h2"},
	{QCF_KK7,		"kkqwsv"},
	{QCF_KK7,		"kk7"},
	{QCF_KK7,		"bigprogs"},
	{QCF_KK7,		"version7"},
	{QCF_KK7,		"kkqwsv"},
	{QCF_FTE,		"fte"},
	{QCF_FTEH2,		"fteh2"},
	{QCF_DARKPLACES,"darkplaces"},
	{QCF_DARKPLACES,"dp"},
	{QCF_QTEST,		"qtest"},
	{0,				NULL}
};


const char *QCC_VersionString(void)
{
#ifdef SVNVERSION
	if (strcmp(SVNVERSION, "-"))
		return "FTEQCC: " STRINGIFY(SVNVERSION) " (" __DATE__")";
#endif
	return "FTEQCC: " __DATE__;
}

/*
=================
BspModels

Runs qbsp and light on all of the models with a .bsp extension
=================
*/
int QCC_CheckParm (char *check);

void QCC_BspModels (void)
{
/*
	int		p;
	char	*gamedir;
	int		i;
	char	*m;
	char	cmd[1024];
	char	name[256];
	size_t result;

	p = QCC_CheckParm ("-bspmodels");
	if (!p)
		return;
	if (p == myargc-1)
		QCC_Error (ERR_BADPARMS, "-bspmodels must preceed a game directory");
	gamedir = myargv[p+1];

	for (i=0 ; i<nummodels ; i++)
	{
		m = precache_models[i];
		if (strcmp(m+strlen(m)-4, ".bsp"))
			continue;
		strcpy (name, m);
		name[strlen(m)-4] = 0;
		sprintf (cmd, "qbsp %s/%s ; light -extra %s/%s", gamedir, name, gamedir, name);
		result = system (cmd); // do something with the result

		if (result != 0)
			QCC_Error(ERR_INTERNAL, "QCC_BspModels() system returned non zero (failure) with: qbsp %s/%s ; light -extra %s/%s (%i)\n", gamedir, name, gamedir, name, errno);
	}
*/
}

typedef struct stringtab_s
{
	struct stringtab_s *prev;
	char *ofs;
	int len;
} stringtab_t;
stringtab_t *stringtablist[256];
// CopyString returns an offset from the string heap
int	QCC_CopyString (char *str)
{
	int		old;

	if (!str)
		return 0;
	if (!*str)
		return !flag_nullemptystr;

	if (opt_noduplicatestrings || !strcmp(str, "IMMEDIATE"))
	{
#if 1
		//more scalable (faster) version.
		stringtab_t *t;
		int len = strlen(str);
		unsigned char key = str[len-1];
		for (t = stringtablist[key]; t; t = t->prev)
		{
			if (t->len >= len)
				if (!strcmp(t->ofs + t->len - len, str))
				{
					optres_noduplicatestrings += len;
					return (t->ofs + t->len - len)-strings;
				}
		}
		t = qccHunkAlloc(sizeof(*t));
		t->prev = stringtablist[key];
		stringtablist[key] = t;
		t->ofs = strings+strofs;
		t->len = len;
#elif 1
		//bruteforce
		char *s;
		for (s = strings; s < strings+strofs; s++)
			if (!strcmp(s, str))
			{
				optres_noduplicatestrings += strlen(str);
				return s-strings;
			}
#endif
	}

	old = strofs;
	strcpy (strings+strofs, str);
	strofs += strlen(str)+1;
	return old;
}

int	QCC_CopyStringLength (char *str, size_t length)
{
	int		old;

	if (!str)
		return 0;
	if (!*str && length == 1)
		return !flag_nullemptystr;

	old = strofs;
	memcpy (strings+strofs, str, length);
	strofs += length;
	return old;
}

int	QCC_CopyDupBackString (char *str)
{
	int		old;
	char *s;

	for (s = strings+strofs-1; s>strings ; s--)
		if (!strcmp(s, str))
			return s-strings;

	old = strofs;
	strcpy (strings+strofs, str);
	strofs += strlen(str)+1;
	return old;
}

void QCC_PrintStrings (void)
{
	int		i, l, j;

	for (i=0 ; i<strofs ; i += l)
	{
		l = strlen(strings+i) + 1;
		printf ("%5i : ",i);
		for (j=0 ; j<l ; j++)
		{
			if (strings[i+j] == '\n')
			{
				putchar ('\\');
				putchar ('n');
			}
			else
				putchar (strings[i+j]);
		}
		printf ("\n");
	}
}


/*void QCC_PrintFunctions (void)
{
	int		i,j;
	QCC_dfunction_t	*d;

	for (i=0 ; i<numfunctions ; i++)
	{
		d = &functions[i];
		printf ("%s : %s : %i %i (", strings + d->s_file, strings + d->s_name, d->first_statement, d->parm_start);
		for (j=0 ; j<d->numparms ; j++)
			printf ("%i ",d->parm_size[j]);
		printf (")\n");
	}
}*/

void QCC_PrintFields (void)
{
	extern char *basictypenames[];
	int		i;
	QCC_ddef_t	*d;

	printf("Fields Listing:\n");

	for (i=0 ; i<numfielddefs ; i++)
	{
		d = &fields[i];
		printf ("%5i : (%s) %s\n", d->ofs, basictypenames[d->type], strings + d->s_name);
	}
}

void QCC_PrintGlobals (void)
{
	int		i;
	QCC_ddef_t	*d;

	printf("Globals Listing:\n");

	for (i=0 ; i<numglobaldefs ; i++)
	{
		d = &qcc_globals[i];
		printf ("%5i : (%i) %s\n", d->ofs, d->type, strings + d->s_name);
	}
}

void QCC_PrintAutoCvars (void)
{
	int		i;
	QCC_ddef_t	*d;
	char *n;

	printf("Auto Cvars:\n");
	for (i=0 ; i<numglobaldefs ; i++)
	{
		d = &qcc_globals[i];
		n = strings + d->s_name;
		if (!strncmp(n, "autocvar_", 9))
		{
			char *desc;
			QCC_eval_t *val = &qcc_pr_globals[d->ofs];
			QCC_def_t *def = QCC_PR_GetDef(NULL, n, NULL, false, 0, 0);
			n += 9;

			if (def->comment)
				desc = def->comment;
			else
				desc = NULL;

			switch(d->type & ~(DEF_SAVEGLOBAL|DEF_SHARED))
			{
			case ev_float:
				printf ("set %s\t%g%s%s\n",				n, val->_float,										desc?"\t//":"", desc?desc:"");
				break;
			case ev_vector:
				printf ("set %s\t\"%g %g %g\"%s%s\n",	n, val->vector[0], val->vector[1], val->vector[2],	desc?"\t//":"", desc?desc:"");
				break;
			case ev_integer:
				printf ("set %s\t%i%s%s\n",				n, val->_int,										desc?"\t//":"", desc?desc:"");
				break;
			case ev_string:
				printf ("set %s\t\"%s\"%s%s\n",			n, strings + val->_int,								desc?"\t//":"", desc?desc:"");
				break;
			default:
				printf ("//set %s\t ?%s%s\n",			n,													desc?"\t//":"", desc?desc:"");
				break;
			}
		}
	}
	printf("\n");
}

void QCC_PrintFiles (void)
{
	struct
	{
		precache_t *list;
		int count;
	} precaches[] =
	{
		{ precache_sound, numsounds},
		{ precache_texture, numtextures},
		{ precache_model, nummodels},
		{ precache_file, numfiles}
	};

	int		g, i, b;
	pbool header;

	printf("\nFile lists:\n");
	for (b = 0; b < 64; b++)
	{
		for (header = false, g = 0; g < sizeof(precaches)/sizeof(precaches[0]); g++)
		{
			for (i = 0; i < precaches[g].count; i++)
			{
				if (precaches[g].list[i].block == b)
				{
					if (*precaches[g].list[i].name == '*')
						continue;	//*-prefixed models are not real, and shouldn't be included in file lists.
					if (!header)
					{
						printf("pak%i:\n", b-1);
						header=true;
					}
					printf("%s\n", precaches[g].list[i].name);
				}
			}
		}
		if (header)
			printf("\n");
	}
}

int encode(int len, int method, char *in, int handle);
int WriteSourceFiles(qcc_cachedsourcefile_t *filelist, int h, pbool sourceaswell, pbool legacyembed)
{
	//helpers to deal with misaligned data. writes little-endian.
#define misbyte(ptr,ofs,data) ((unsigned char*)(ptr))[ofs] = (data)&0xff;
#define misshort(ptr,ofs,data) misbyte((ptr),(ofs),(data));misbyte((ptr),(ofs)+1,(data)>>8);
#define misint(ptr,ofs,data) misshort((ptr),(ofs),(data));misshort((ptr),(ofs)+2,(data)>>16);
	includeddatafile_t *idf;
	qcc_cachedsourcefile_t *f;
	int num=0;
	int ofs;
	pbool zipembed = true;
	int startofs;
	sourceaswell |= flag_embedsrc;

	for (f = filelist,num=0; f ; f=f->next)
	{
		if (f->type == FT_CODE && !sourceaswell)
			continue;
		num++;
	}
	if (!num)
	{
		if (zipembed)
		{	//zips are found by scanning. so make sure something can be found so noone will erroneously find something
			char centralheader[22];
			int centraldirofs = SafeSeek(h, 0, SEEK_CUR);
			misint  (centralheader, 0, 0x06054b50);
			misshort(centralheader, 4, 0);	//this disk number
			misshort(centralheader, 6, 0);	//centraldir first disk
			misshort(centralheader, 8, 0);	//centraldir entries
			misshort(centralheader, 10, 0);	//total centraldir entries
			misint  (centralheader, 12, 0);	//centraldir size
			misint  (centralheader, 16, centraldirofs);	//centraldir offset
			misshort(centralheader, 20, 0);	//comment length
			SafeWrite(h, centralheader, 22);
		}
		return 0;
	}
	startofs = SafeSeek(h, 0, SEEK_CUR);
	idf = qccHunkAlloc(sizeof(includeddatafile_t)*num);
	for (f = filelist,num=0; f ; f=f->next)
	{
		if (f->type == FT_CODE && !sourceaswell)
			continue;

		if (zipembed)
		{
			size_t end;
			char header[32+sizeof(f->filename)];
			size_t fnamelen = strlen(f->filename);
			f->zcrc = QC_encodecrc(f->size, f->file);
			misint  (header, 0, 0x04034b50);
			misshort(header, 4, 0);//minver
			misshort(header, 6, 0);//general purpose flags
			misshort(header, 8, 0);//compression method, 0=store, 8=deflate
			misshort(header, 10, 0);//lastmodfiletime
			misshort(header, 12, 0);//lastmodfiledate
			misint  (header, 14, f->zcrc);//crc32
			misint  (header, 18, f->size);//compressed size
			misint  (header, 22, f->size);//uncompressed size
			misshort(header, 26, fnamelen);//filename length
			misshort(header, 28, 0);//extradata length
			strcpy(header+30, f->filename);

			f->zhdrofs = SafeSeek(h, 0, SEEK_CUR);
			SafeWrite(h, header, 30+fnamelen);

			strcpy(idf[num].filename, f->filename);
			idf[num].size = f->size;
			idf[num].compmethod = 8;	//must be 0(raw) or 8(raw deflate) for zips to work
			idf[num].ofs = SafeSeek(h, 0, SEEK_CUR);
			if (idf[num].compmethod==0)
				SafeWrite(h, f->file, f->size);
			else
			{
				idf[num].compsize = QC_encode(progfuncs, f->size, idf[num].compmethod, f->file, h);

				misshort(header, 8, idf[num].compmethod);//compression method, 0=store, 8=deflate
				misint  (header, 18, idf[num].compsize);

				end = SafeSeek(h, 0, SEEK_CUR);
				SafeSeek(h, f->zhdrofs, SEEK_SET);
				SafeWrite(h, header, 30+strlen(f->filename));
				SafeSeek(h, end, SEEK_SET);
			}
		}
		else
		{
			if (f->type == FT_CODE && !sourceaswell)
				continue;

			strcpy(idf[num].filename, f->filename);
			idf[num].size = f->size;
	#ifdef AVAIL_ZLIB
			idf[num].compmethod = 2;
	#else
			idf[num].compmethod = 0;
	#endif
			idf[num].ofs = SafeSeek(h, 0, SEEK_CUR);
			idf[num].compsize = QC_encode(progfuncs, f->size, idf[num].compmethod, f->file, h);
		}
		num++;
	}

	if (zipembed)
	{
		char centralheader[46+sizeof(f->filename)];
		int centraldirsize;
		ofs = SafeSeek(h, 0, SEEK_CUR);
		for (f = filelist,num=0; f ; f=f->next)
		{
			size_t fnamelen;
			if (f->type == FT_CODE && !sourceaswell)
				continue;
			fnamelen = strlen(f->filename);
			misint  (centralheader, 0, 0x02014b50);
			misshort(centralheader, 4, 0);//ourver
			misshort(centralheader, 6, 0);//minver
			misshort(centralheader, 8, 0);//general purpose flags
			misshort(centralheader, 10, idf[num].compmethod);//compression method, 0=store, 8=deflate
			misshort(centralheader, 12, 0);//lastmodfiletime
			misshort(centralheader, 14, 0);//lastmodfiledate
			misint  (centralheader, 16, f->zcrc);//crc32
			misint  (centralheader, 20, idf[num].compsize);//compressed size
			misint  (centralheader, 24, f->size);//uncompressed size
			misshort(centralheader, 28, fnamelen);//filename length
			misshort(centralheader, 30, 0);//extradata length
			misshort(centralheader, 32, 0);//comment length
			misshort(centralheader, 34, 0);//first disk number
			misshort(centralheader, 36, 0);//internal file attribs
			misint  (centralheader, 38, 0);//external file attribs
			misint  (centralheader, 42, f->zhdrofs);//local header offset
			strcpy(centralheader+46, f->filename);
			SafeWrite(h, centralheader, 46 + fnamelen);
			num++;
		}

		centraldirsize = SafeSeek(h, 0, SEEK_CUR)-ofs;
		misint  (centralheader, 0, 0x06054b50);
		misshort(centralheader, 4, 0);	//this disk number
		misshort(centralheader, 6, 0);	//centraldir first disk
		misshort(centralheader, 8, num);	//centraldir entries
		misshort(centralheader, 10, num);	//total centraldir entries
		misint  (centralheader, 12, centraldirsize);	//centraldir size
		misint  (centralheader, 16, ofs);	//centraldir offset
		misshort(centralheader, 20, 0);	//comment length
		SafeWrite(h, centralheader, 22);

		ofs = 0;
	}
	else if (legacyembed)
	{
		ofs = SafeSeek(h, 0, SEEK_CUR);
		SafeWrite(h, &num, sizeof(int));
		SafeWrite(h, idf, sizeof(includeddatafile_t)*num);
	}
	else
		ofs = 0;

	printf("Embedded files take %u bytes\n",  SafeSeek(h, 0, SEEK_CUR) - startofs);

	return ofs;
}

void QCC_InitData (void)
{
	static char parmname[12][MAX_PARMS];
	int		i;

	qcc_sourcefile = NULL;

	memset(stringtablist, 0, sizeof(stringtablist));
	numstatements = 1;	//first statement should be an OP_DONE, matching the null function(ish), in case it somehow doesn't get caught by the vm.
	strofs = 2;			//null, empty, *other stuff*
	numfunctions = 1;	//first function is a null function.
	numglobaldefs = 1;	//first globaldef is a null def. just because. doesn't include parms+ret. is there any point to this?

	numfielddefs = 0;
	fields[numfielddefs].type = ev_void;
	fields[numfielddefs].s_name = 0;	//should map to null
	fields[numfielddefs].ofs = 0;
	numfielddefs++;	//FIXME: do we actually need a null field? is there any point to this at all?

	memset(&def_ret, 0, sizeof(def_ret));
	def_ret.ofs = OFS_RETURN;
	def_ret.name = "return";
	def_ret.constant = false;
	def_ret.type	= NULL;
	def_ret.symbolheader = &def_ret;
	for (i=0 ; i<MAX_PARMS ; i++)
	{
		def_parms[i].symbolheader = &def_parms[i];
		def_parms[i].temp = NULL;
		def_parms[i].type = NULL;
		def_parms[i].ofs = OFS_PARM0 + 3*i;
		def_parms[i].name = parmname[i];
		sprintf(parmname[i], "parm%i", i);
	}
}

int WriteBodylessFuncs (int handle)
{
	QCC_def_t		*d;
	int ret=0;
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!d->used || !d->constant)
			continue;

		if (d->type->type == ev_function && !d->scope)// function parms are ok
		{
			if (d->initialized == 2)
			{
				SafeWrite(handle, d->name, strlen(d->name)+1);
				ret++;
			}
			if (d->initialized == 0)
			{
				QCC_PR_Warning(ERR_NOFUNC, d->filen, d->s_line, "function %s has no body", d->name);
				QCC_PR_ParsePrintDef(ERR_NOFUNC, d);
			}
		}
	}

	return ret;
}

void QCC_DetermineNeededSymbols(QCC_def_t *endsyssym)
{
	QCC_def_t *sym = pr.def_head.next;
	size_t i;
	//make sure system defs are not hurt by this.
	if (endsyssym)
	{
		for (; sym; sym = sym->next)
		{
			sym->used = true;
			sym->referenced = true;	//silence warnings about unreferenced things that can't be stripped
			if (sym == endsyssym)
				break;
		}
	}
	//non-system fields should maybe be present too.
	for (; sym; sym = sym->next)
	{
		if (sym->constant && sym->type->type == ev_field && !opt_stripunusedfields)
			sym->symbolheader->used = true;	//fields should always be present, annoyingly enough, as they're often used to silence map warnings.
											//FIXME: we want to strip these. fte/dp extensions.qc have a LOT of fields that could be stripped.
		else if (sym->constant && sym->type->type == ev_function)
		{						//non-builtins must be present, because of spawn functions and other entrypoints.
			unsigned int fnum = sym->symboldata[sym->ofs]._int;
			if (fnum < numfunctions && functions[fnum].code == -1)
				sym->symbolheader->used = true;
		}
	}

	for (i=0 ; i<numstatements ; i++)
	{
		if ((sym = statements[i].a.sym))
			if (sym->symbolheader)
				sym->symbolheader->used = true;
		if ((sym = statements[i].b.sym))
			if (sym->symbolheader)
				sym->symbolheader->used = true;
		if ((sym = statements[i].c.sym))
			if (sym->symbolheader)
				sym->symbolheader->used = true;
	}
}

//allocates final space for the def, making it a true def
void QCC_FinaliseDef(QCC_def_t *def)
{
	if (def->symbolheader == def && def->deftail)
	{
		//for head symbols, we go through and touch all of their children
		QCC_def_t *prev, *sub;
		if (!def->referenced)
		{
			pbool ignoreone = true;
			//touch all but one child
			for (prev = def, sub = prev->next; prev != def->deftail; sub = (prev=sub)->next)
			{
				if (sub->referenced)
					def->referenced=true;	//if one child is referenced, the composite is referenced
				else if (!sub->referenced && ignoreone)
					ignoreone = false;	//this is the one we're going to warn about
				else
					sub->referenced |= def->referenced;			
			}
			if (!def->referenced)	//no child defs were referenced at all. if we're going to be warning about this then at least mute warnings for any other fields
				for (prev = def, sub = prev->next; prev != def->deftail; sub = (prev=sub)->next)
					sub->referenced = true;
		}
		else
		{
			//touch children
			for (prev = def, sub = prev->next; prev != def->deftail; sub = (prev=sub)->next)
				sub->referenced |= def->referenced;
		}
	}
//	else if (def->symbolheader)	//if a child symbol is referenced, mark the entire parent as referenced too. this avoids vec_x+vec_y with no vec or vec_z from generating warnings about vec being unreferenced
//		def->symbolheader->referenced |= def->referenced;

	if (!def->symbolheader->used)
	{
		if (verbose >= 3)
			printf("not needed: %s\n", def->name);
		return;
	}

	else if (def->symbolheader != def)
		def->ofs += def->symbolheader->ofs;
	else
	{
		if (def->arraylengthprefix)
		{
			if (numpr_globals+1+def->symbolsize >= MAX_REGS)
			{
				if (!opt_overlaptemps || !opt_locals_overlapping)
					QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS - you'll need to use more optimisations");
				else
					QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS");
			}

			//for hexen2 arrays, we need to emit a length first
			if (def->type->type == ev_vector)
				((int *)qcc_pr_globals)[numpr_globals] = def->arraysize-1;
			else
				((int *)qcc_pr_globals)[numpr_globals] = (def->arraysize*def->type->size)-1;	//using float arrays for structs.
			numpr_globals+=1;
		}
		else if (numpr_globals+def->symbolsize >= MAX_REGS)
		{
			if (!opt_overlaptemps || !opt_locals_overlapping)
				QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS - you'll need to use more optimisations");
			else
				QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS");
		}
		def->ofs += numpr_globals;
		numpr_globals += def->symbolsize;

		if (def->symboldata)
			memcpy(qcc_pr_globals+def->ofs, def->symboldata, def->symbolsize*sizeof(float));
		else
			memset(qcc_pr_globals+def->ofs, 0, def->symbolsize*sizeof(float));
	}
	def->symboldata = qcc_pr_globals;
	def->symbolsize = numpr_globals;

#ifdef DEBUG_DUMP
	if (!def->referenced)
		printf("Unreferenced ");
	printf("Finalise %s(%p) @ %i+%i\n", def->name, def, def->ofs, ssize);
#endif
}

//marshalled locals still point to the FIRST_LOCAL range.
//this function remaps all the locals back into actual usable defs.
void QCC_UnmarshalLocals(void)
{
	QCC_def_t *d;
	unsigned int onum, biggest, eog = numpr_globals;
	size_t i;
	//first, finalize private locals.
	for (i=0 ; i<numfunctions ; i++)
	{
		if (functions[i].privatelocals)
		{
			onum = numpr_globals;
#ifdef DEBUG_DUMP
			printf("function %s locals:\n", functions[i].name);
#endif
			for (d = functions[i].firstlocal; d; d = d->nextlocal)
				QCC_FinaliseDef(d);
			if (verbose >= 3)
			{
				if (onum == numpr_globals)
					printf("code: %s:%i: function %s no private locals\n", functions[i].filen, functions[i].line, functions[i].name);
				else 
					printf("code: %s:%i: function %s private locals %i-%i\n", functions[i].filen, functions[i].line, functions[i].name, onum, numpr_globals);
			}
		}
	}

	//these functions don't have any initialisation issues, allowing us to merge them
	onum = biggest = numpr_globals;
	for (i=0 ; i<numfunctions ; i++)
	{
		if (!functions[i].privatelocals)
		{
			numpr_globals = onum;
			for (d = functions[i].firstlocal; d; d = d->nextlocal)
				QCC_FinaliseDef(d);
			if (biggest < numpr_globals)
				biggest = numpr_globals;

			if (verbose >= 3)
			{
				if (onum == numpr_globals)
					printf("code: %s:%i: function %s no locals\n", functions[i].filen, functions[i].line, functions[i].name);
				else
				{
					printf("code: %s:%i: function %s overlapped locals %i-%i\n", functions[i].filen, functions[i].line, functions[i].name, onum, numpr_globals);

					for (d = functions[i].firstlocal; d; d = d->nextlocal)
					{
						printf("code: %s:%i: %s @%i\n", functions[i].filen, functions[i].line, d->name, d->ofs);
					}
				}
			}
		}
	}
	numpr_globals = biggest;
	if (verbose)
		printf("%i shared locals, %i private, %i total\n", biggest - onum, onum - eog, numpr_globals-eog);
}
static void QCC_GenerateFieldDefs(QCC_def_t *def, char *fieldname, int ofs, QCC_type_t *type)
{
	string_t sname;
	QCC_ddef32_t *dd;
	if (type->type == ev_struct || type->type == ev_union)
	{	//the qcvm cannot cope with struct fields. so we need to generate lots of fake ones.
		char sub[256];
		unsigned int p, a;
		int parms = type->num_parms;
		if (type->type == ev_union)
			parms = 1;	//unions only generate the first element. it simplifies things (should really just be the biggest).
		for (p = 0; p < parms; p++)
		{
			if (type->params[p].arraysize)
			{
				for (a = 0; a < type->params[p].arraysize; a++)
				{
					QC_snprintfz(sub, sizeof(sub), "%s.%s[%u]", fieldname, type->params[p].paramname, a);
					QCC_GenerateFieldDefs(def, sub, ofs + type->params[p].ofs + a * type->params[p].type->size, type->params[p].type);
				}
			}
			else
			{
				QC_snprintfz(sub, sizeof(sub), "%s.%s", fieldname, type->params[p].paramname);
				QCC_GenerateFieldDefs(def, sub, ofs + type->params[p].ofs, type->params[p].type);
			}
		}
		return;
	}
	if (numfielddefs >= MAX_FIELDS)
		QCC_PR_ParseError(0, "Too many fields. Limit is %u\n", MAX_FIELDS);
	dd = &fields[numfielddefs];
	numfielddefs++;
	dd->type = type->type;
	dd->s_name = sname = QCC_CopyString (fieldname);
	dd->ofs = def->symboldata[ofs]._int;

	if (numglobaldefs >= MAX_GLOBALS)
		QCC_PR_ParseError(0, "Too many globals. Limit is %u\n", MAX_GLOBALS);
	//and make sure that there's a global defined too, so field remapping isn't screwed.
	dd = &qcc_globals[numglobaldefs];
	numglobaldefs++;
	dd->type = ev_field;
	dd->ofs = ofs;
	dd->s_name = sname;
}


CompilerConstant_t *QCC_PR_CheckCompConstDefined(char *def);
pbool QCC_WriteData (int crc)
{
	QCC_def_t		*def;
	QCC_ddef_t		*dd;
	dprograms_t	progs;
	int			h;
	int			i, len;
	pbool debugtarget = false;
	pbool types = false;
	int outputsttype = PST_DEFAULT;
	int dupewarncount = 0, extwarncount = 0;
	int			*statement_linenums;
	void		*funcdata;
	size_t		funcdatasize;


	extern char *basictypenames[];

	memset(&progs, 0, sizeof(progs));

	if (numstatements==1 && numfunctions==1 && numglobaldefs==1 && numfielddefs==1)
	{
		printf("nothing to write\n");
		return false;
	}

	progs.blockscompressed=0;

	if (numstatements > MAX_STATEMENTS)
		QCC_Error(ERR_TOOMANYSTATEMENTS, "Too many statements - %i\nAdd \"MAX_STATEMENTS\" \"%i\" to qcc.cfg", numstatements, (numstatements+32768)&~32767);

	if (strofs > MAX_STRINGS)
		QCC_Error(ERR_TOOMANYSTRINGS, "Too many strings - %i\nAdd \"MAX_STRINGS\" \"%i\" to qcc.cfg", strofs, (strofs+32768)&~32767);

	//part of how compilation works. This def is always present, and never used.
	def = QCC_PR_GetDef(NULL, "end_sys_globals", NULL, false, 0, false);
	if (def)
		def->referenced = true;

	def = QCC_PR_GetDef(NULL, "end_sys_fields", NULL, false, 0, false);
	if (def)
		def->referenced = true;
	QCC_DetermineNeededSymbols(def);

	for (def = pr.def_head.next ; def ; def = def->next)
	{
		if (!def->localscope)
			QCC_FinaliseDef(def);
	}
	QCC_UnmarshalLocals();
	QCC_FinaliseTemps();

	switch (qcc_targetformat)
	{
	case QCF_HEXEN2:
	case QCF_STANDARD:
		if (bodylessfuncs)
			printf("Warning: There are some functions without bodies.\n");

		if (numpr_globals > 65530 )
		{
			printf("Forcing target to FTE32 due to numpr_globals\n");
			outputsttype = PST_FTE32;
		}
		else if (qcc_targetformat == QCF_FTEH2)
		{
			printf("Progs execution will require FTE\n");
			break;
		}
		else if (qcc_targetformat == QCF_HEXEN2)
		{
			printf("Progs execution requires a Hexen2 compatible HCVM\n");
			break;
		}
		else
		{
			if (numpr_globals >= 32768)	//not much of a different format. Rewrite output to get it working on original executors?
				printf("Globals exceeds 32k - an enhanced QCVM will be required\n");
			else if (verbose)
				printf("Progs should run on any QuakeC VM\n");
			break;
		}
		qcc_targetformat = (qcc_targetformat==QCF_HEXEN2)?QCF_FTEH2:QCF_FTE;
		//intentional fallthrough
	case QCF_FTEDEBUG:
	case QCF_FTE:
	case QCF_FTEH2:
	case QCF_DARKPLACES:
		if (qcc_targetformat == QCF_FTEDEBUG)
			debugtarget = true;

		if (numpr_globals > 65530)
		{
			printf("Using 32 bit target due to numpr_globals\n");
			outputsttype = PST_FTE32;
		}

		if (qcc_targetformat == QCF_DARKPLACES)
			compressoutput = 0;


		//compression of blocks?
		if (compressoutput)		progs.blockscompressed |=1;		//statements
		if (compressoutput)		progs.blockscompressed |=2;		//defs
		if (compressoutput)		progs.blockscompressed |=4;		//fields
		if (compressoutput)		progs.blockscompressed |=8;		//functions
		if (compressoutput)		progs.blockscompressed |=16;	//strings
		if (compressoutput)		progs.blockscompressed |=32;	//globals
		if (compressoutput)		progs.blockscompressed |=64;	//line numbers
		if (compressoutput)		progs.blockscompressed |=128;	//types
		//include a type block?
		//types = debugtarget;
		if (types && sizeof(char *) != sizeof(string_t))
		{
			//qcc_typeinfo_t has a char* inside it, which changes size
			printf("AMD64 builds cannot write typeinfo structures\n");
			types = false;
		}

		if (verbose)
		{
			if (qcc_targetformat == QCF_DARKPLACES)
				printf("DarkPlaces or FTE will be required\n");
			else
				printf("An FTE executor will be required\n");
		}
		break;
	case QCF_KK7:
		if (bodylessfuncs)
			printf("Warning: There are some functions without bodies.\n");
		if (numpr_globals > 65530)
			printf("Warning: Saving is not supported. Ensure all engine read fields and globals are defined early on.\n");

		printf("A KK compatible executor will be required (FTE/KK)\n");
		outputsttype = PST_KKQWSV;
		break;
	case QCF_QTEST:
		printf("Compiled QTest progs will most likely not work at all. YOU'VE BEEN WARNED!\n");
		outputsttype = PST_QTEST;
		break;
	default:
		Sys_Error("invalid progs type chosen!");
	}


	switch (outputsttype)
	{
	case PST_QTEST:
		{
			// this sucks but the structures are just too different
			qtest_function_t *funcs = (qtest_function_t *)qccHunkAlloc(sizeof(qtest_function_t)*numfunctions);

			for (i=0 ; i<numfunctions ; i++)
			{
				int j;

				funcs[i].unused1 = 0;
				funcs[i].profile = 0;
				if (functions[i].code == -1)
					funcs[i].first_statement = PRLittleLong (-functions[i].builtin);
				else
					funcs[i].first_statement = PRLittleLong (functions[i].code);
				funcs[i].first_statement = PRLittleLong (functions[i].code);
				funcs[i].parm_start = 0;//PRLittleLong (functions[i].parm_start);
				funcs[i].s_name = PRLittleLong (QCC_CopyString(functions[i].name));
				funcs[i].s_file = PRLittleLong (functions[i].s_filed);
				funcs[i].numparms = 0;//PRLittleLong ((functions[i].numparms>MAX_PARMS)?MAX_PARMS:functions[i].numparms);
				funcs[i].locals = 0;//PRLittleLong (functions[i].locals);
				for (j = 0; j < MAX_PARMS; j++)
					funcs[i].parm_size[j] = 0;//PRLittleLong((int)functions[i].parm_size[j]);
			}
			funcdata = funcs;
			funcdatasize = numfunctions*sizeof(*funcs);
		}
		break;
	case PST_DEFAULT:
	case PST_KKQWSV:
	case PST_FTE32:
		{
			dfunction_t *funcs = (dfunction_t *)qccHunkAlloc(sizeof(dfunction_t)*numfunctions);
			for (i=0 ; i<numfunctions ; i++)
			{
				QCC_def_t *local;
				unsigned int p, a;
				if (functions[i].code == -1)
					funcs[i].first_statement = PRLittleLong (-functions[i].builtin);
				else
					funcs[i].first_statement = PRLittleLong (functions[i].code);
				if (opt_function_names && functions[i].builtin)
				{
					//builtins don't need a name entry. the def is constant, so no string need be emitted at all.
					optres_function_names += strlen(functions[i].name)+1;
					funcs[i].s_name = 0;
				}
				else
					funcs[i].s_name = PRLittleLong (QCC_CopyString(functions[i].name));
				funcs[i].s_file = PRLittleLong (functions[i].s_filed);

				if (functions[i].merged)
				{
					funcs[i].parm_start = functions[i].merged->parm_start;
					funcs[i].locals = functions[i].merged->locals;
					funcs[i].numparms = functions[i].merged->numparms;
					for(p = 0; p < funcs[i].numparms; p++)
						funcs[i].parm_size[p] = functions[i].merged->parm_size[p];
				}
				else if (functions[i].code == -1)
				{
					funcs[i].parm_start = 0;
					funcs[i].locals = 0;
					funcs[i].numparms = functions[i].type->num_parms;
					if (funcs[i].numparms > MAX_PARMS)
						funcs[i].numparms = MAX_PARMS;	//shouldn't happen for builtins.
					p = 0;
				}
				else if (functions[i].firstlocal)
				{
					unsigned int size;
					funcs[i].parm_start = 0;
					for (local = functions[i].firstlocal, a = 0, p = 0; local && a < MAX_PARMS && a < functions[i].type->num_parms; local = local->deftail->nextlocal)
					{
						if (!local->used)
						{	//all params should have been assigned space. logically we could have safely omitted the last ones, but blurgh.
							QCC_PR_Warning(ERR_INTERNAL, local->filen, local->s_line, "Argument %s was not marked used.\n", local->name);
							continue;
						}

						if (!p)
							funcs[i].parm_start = local->ofs;

						size = local->type->size;
						if (local->arraysize)	//arrays are annoying
							size = local->arraylengthprefix+size*local->arraysize;

						funcs[i].locals += size;
						for (; size > 3 && p < MAX_PARMS; size -= 3)
							funcs[i].parm_size[p++] = 3;	//the engine will copy PARMi over, it can't cope with larger types, the following args would be wrong.
						if (p < MAX_PARMS)
							funcs[i].parm_size[p++] = size;
						a++;
					}
					for (; local && !local->used; local = local->nextlocal)
						;
					if (!p && local)
						funcs[i].parm_start = local->ofs;
					for (; local; local = local->nextlocal)
					{
						size = local->type->size;
						if (local->arraysize)	//arrays are annoying
							size = local->arraylengthprefix+size*local->arraysize;

						if (local->used)
							funcs[i].locals += size;
					}
					funcs[i].numparms = p;
				}
				else
				{
					//fucked up functions with no params.
					funcs[i].parm_start = 0;
					funcs[i].locals = 0;
					funcs[i].numparms = 0;
					p = 0;
				}
				while(p < MAX_PARMS)
					funcs[i].parm_size[p++] = 0;
				funcs[i].parm_start = PRLittleLong(funcs[i].parm_start);
				funcs[i].locals = PRLittleLong(funcs[i].locals);
				funcs[i].numparms = PRLittleLong(funcs[i].numparms);

				if (funcs[i].locals && !funcs[i].parm_start)
					QCC_PR_Warning(0, strings + funcs[i].s_file, functions[i].line, "%s:%i: func %s @%i locals@%i+%i, %i parms\n", functions[i].filen, functions[i].line, strings+funcs[i].s_name, funcs[i].first_statement, funcs[i].parm_start, funcs[i].locals, funcs[i].numparms);

#ifdef DEBUG_DUMP
				printf("code: %s:%i: func %s @%i locals@%i+%i, %i parms\n", functions[i].file, functions[i].line, strings+funcs[i].s_named, funcs[i].first_statement, funcs[i].parm_start, funcs[i].locals, funcs[i].numparms);
#endif
			}
			funcdata = funcs;
			funcdatasize = numfunctions*sizeof(*funcs);
		}
		break;
	default:
		Sys_Error("structtype error");
	}

	for (dupewarncount = 0, def = pr.def_head.next ; def ; def = def->next)
	{
/*		if (def->type->type == ev_vector || (def->type->type == ev_field && def->type->aux_type->type == ev_vector))
		{	//do the references, so we don't get loadsa not referenced VEC_HULL_MINS_x
			s_file = def->s_file;
			QC_snprintfz (element, sizeof(element), "%s_x", def->name);
			comp_x = QCC_PR_GetDef(NULL, element, def->scope, false, 0, false);
			QC_snprintfz (element, sizeof(element), "%s_y", def->name);
			comp_y = QCC_PR_GetDef(NULL, element, def->scope, false, 0, false);
			QC_snprintfz (element, sizeof(element), "%s_z", def->name);
			comp_z = QCC_PR_GetDef(NULL, element, def->scope, false, 0, false);

			h = def->references;
			if (comp_x && comp_y && comp_z)
			{
				h += comp_x->references;
				h += comp_y->references;
				h += comp_z->references;

				if (!def->references)
					if (!comp_x->references || !comp_y->references || !comp_z->references)	//one of these vars is useless...
						h=0;

				def->references = h;


				if (!h)
					h = 1;
				if (comp_x)
					comp_x->references = h;
				if (comp_y)
					comp_y->references = h;
				if (comp_z)
					comp_z->references = h;
			}
		}
*/
		if (!def->symbolheader->used)
		{
			int wt = def->constant?WARN_NOTREFERENCEDCONST:WARN_NOTREFERENCED;
			pr_scope = def->scope;
			if (!strncmp(def->name, "spawnfunc_", 10))
				;	//no warnings from unreferenced entry points.
			else if (strcmp(def->name, "IMMEDIATE") && qccwarningaction[wt] && !(def->type->type == ev_function && def->symbolheader->timescalled) && !def->symbolheader->used)
			{
				char typestr[256];
				if (QC_strcasestr(def->filen, "extensions") && !verbose)
				{	//try to avoid annoying warnings from dpextensions.qc
					extwarncount++;
					QCC_PR_Warning(wt, def->filen, def->s_line, NULL);
				}
				else if (def->arraysize)
					QCC_PR_Warning(wt, def->filen, def->s_line, (dupewarncount++ >= 10 && !verbose)?NULL:"%s %s[%i]  no references.", TypeName(def->type, typestr, sizeof(typestr)), def->name, def->arraysize);
				else
					QCC_PR_Warning(wt, def->filen, def->s_line, (dupewarncount++ >= 10 && !verbose)?NULL:"%s %s  no references.", TypeName(def->type, typestr, sizeof(typestr)), def->name);
			}
			pr_scope = NULL;

			if (opt_unreferenced && def->type->type != ev_field)
			{
				optres_unreferenced++;
#ifdef DEBUG_DUMP
				printf("code: %s:%i: strip noref %s %s@%i;\n", def->filen, def->s_line, def->type->name, def->name, def->ofs);
#endif
				continue;
			}
		}
		if ((def->type->type == ev_struct || def->type->type == ev_union || def->arraysize) && def->deftail)
		{
#ifdef DEBUG_DUMP
			printf("code: %s:%i: strip struct %s %s@%i;\n", def->filen, def->s_line, def->type->name, def->name, def->ofs);
#endif
			//the head of an array/struct is never written. only, its member fields are.
			continue;
		}

		if (def->strip || !def->symbolheader->used)
		{
#ifdef DEBUG_DUMP
			printf("code: %s:%i: strip %s %s@%i;\n", def->filen, def->s_line, def->type->name, def->name, def->ofs);
#endif
			continue;
		}

		if (def->type->type == ev_function)
		{
			if (opt_function_names && def->initialized && functions[def->symboldata[def->ofs].function].code<0)
			{
				optres_function_names++;
				def->name = "";
			}
#if IAMNOTLAZY
			if (!def->timescalled)
			{
				if (def->references<=1 && strncmp(def->name, "spawnfunc_", 10))
					QCC_PR_Warning(WARN_DEADCODE, strings + def->s_file, def->s_line, "%s is never directly called or referenced (spawn function or dead code)", def->name);
//				else
//					QCC_PR_Warning(WARN_DEADCODE, strings + def->s_file, def->s_line, "%s is never directly called", def->name);
			}
			if (opt_stripfunctions && def->constant && def->timescalled >= def->references-1)	//make sure it's not copied into a different var.
			{								//if it ever does self.think then it could be needed for saves.
				optres_stripfunctions++;	//if it's only ever called explicitly, the engine doesn't need to know.
#ifdef DEBUG_DUMP
				printf("code: %s:%i: strip const %s %s@%i;\n", strings+def->s_file, def->s_line, def->type->name, def->name, def->ofs);
#endif
				continue;
			}
#endif
		}
		else if (def->type->type == ev_field && def->constant && (!def->scope || def->isstatic || def->initialized))
		{
			QCC_GenerateFieldDefs(def, def->name, def->ofs, def->type->aux_type);
			continue;
		}
		else if ((def->scope||def->constant) && (def->type->type != ev_string || (strncmp(def->name, "dotranslate_", 12) && opt_constant_names_strings)))
		{
			if (opt_constant_names)
			{
				if (def->type->type == ev_string)
					optres_constant_names_strings += strlen(def->name);
				else
					optres_constant_names += strlen(def->name);

#ifdef DEBUG_DUMP
				if (def->scope)
					printf("code: %s:%i: strip local %s %s@%i;\n", def->filen, def->s_line, def->type->name, def->name, def->ofs);
				else if (def->constant)
					printf("code: %s:%i: strip const %s %s@%i;\n", def->filen, def->s_line, def->type->name, def->name, def->ofs);
#endif
				continue;
			}
		}

//		if (!def->saved && def->type->type != ev_string)
//			continue;
		dd = &qcc_globals[numglobaldefs];
		numglobaldefs++;

		if (types)
			dd->type = def->type-qcc_typeinfo;
		else
			dd->type = def->type->type;
#ifdef DEF_SAVEGLOBAL
		if ( def->saved && !def->constant// || def->type->type == ev_function)
//		&& def->type->type != ev_function
		&& def->type->type != ev_field
		&& def->scope == NULL)
		{
			dd->type |= DEF_SAVEGLOBAL;
		}
#endif
		if (def->shared)
			dd->type |= DEF_SHARED;

		if (opt_locals && (def->scope || !strcmp(def->name, "IMMEDIATE")))
		{
			dd->s_name = 0;
			optres_locals += strlen(def->name);
		}
		else
			dd->s_name = QCC_CopyString (def->name);
		dd->ofs = def->ofs;

#ifdef DEBUG_DUMP
		if ((dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_string)
			printf("code: %s:%i: %s%s%s %s@%i = \"%s\"\n", def->filen, def->s_line, dd->type&DEF_SAVEGLOBAL?"save ":"nosave ", dd->type&DEF_SHARED?"shared ":"", basictypenames[dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)], strings+dd->s_name, dd->ofs, ((unsigned)def->symboldata[def->ofs].string>=(unsigned)strofs)?"???":(strings + def->symboldata[def->ofs].string));
		else if ((dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_float)
			printf("code: %s:%i: %s%s%s %s@%i = %g\n", def->filen, def->s_line, dd->type&DEF_SAVEGLOBAL?"save ":"nosave ", dd->type&DEF_SHARED?"shared ":"", basictypenames[dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)], strings+dd->s_name, dd->ofs, def->symboldata[def->ofs]._float);
		else if ((dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_integer)
			printf("code: %s:%i: %s%s%s %s@%i = %i\n", def->filen, def->s_line, dd->type&DEF_SAVEGLOBAL?"save ":"nosave ", dd->type&DEF_SHARED?"shared ":"", basictypenames[dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)], strings+dd->s_name, dd->ofs, def->symboldata[def->ofs]._int);
		else if ((dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_vector)
			printf("code: %s:%i: %s%s%s %s@%i = '%g %g %g'\n", def->filen, def->s_line, dd->type&DEF_SAVEGLOBAL?"save ":"nosave ", dd->type&DEF_SHARED?"shared ":"", basictypenames[dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)], strings+dd->s_name, dd->ofs, def->symboldata[def->ofs].vector[0], def->symboldata[def->ofs].vector[1], def->symboldata[def->ofs].vector[2]);
		else if ((dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_function)
			printf("code: %s:%i: %s%s%s %s@%i = %i(%s)\n", def->filen, def->s_line, dd->type&DEF_SAVEGLOBAL?"save ":"nosave ", dd->type&DEF_SHARED?"shared ":"", basictypenames[dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)], strings+dd->s_name, dd->ofs, def->symboldata[def->ofs].function, def->symboldata[def->ofs].function >= numfunctions?"???":functions[def->symboldata[def->ofs].function].name);
		else if ((dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)) == ev_field)
			printf("code: %s:%i: %s%s%s %s@%i = @%i\n", def->filen, def->s_line, dd->type&DEF_SAVEGLOBAL?"save ":"nosave ", dd->type&DEF_SHARED?"shared ":"", basictypenames[dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)], strings+dd->s_name, dd->ofs, def->symboldata[def->ofs]._int);
		else
			printf("code: %s:%i: %s%s%s %s@%i\n", def->filen, def->s_line, dd->type&DEF_SAVEGLOBAL?"save ":"nosave ", dd->type&DEF_SHARED?"shared ":"", basictypenames[dd->type&~(DEF_SHARED|DEF_SAVEGLOBAL)], strings+dd->s_name, dd->ofs);
#endif
	}

	if (dupewarncount > 10 && !verbose)
		QCC_PR_Note(WARN_NOTREFERENCED, NULL, 0, "suppressed %i more warnings about unreferenced variables, as you clearly don't care about the first 10.", dupewarncount-10);
	if (extwarncount)
		QCC_PR_Note(WARN_NOTREFERENCED, NULL, 0, "suppressed %i warnings about unused extensions.", extwarncount);

	for (i = 0; i < numglobaldefs; i++)
	{
		dd = &qcc_globals[i];
		if (!(dd->type & DEF_SAVEGLOBAL))	//only warn about saved ones.
			continue;

		for (h = 0; h < numglobaldefs; h++)
		{
			if (i == h || !(qcc_globals[h].type & DEF_SAVEGLOBAL))
				continue;
			if (dd->ofs == qcc_globals[h].ofs)
			{
				if ((dd->type&~DEF_SAVEGLOBAL) != (qcc_globals[h].type&~DEF_SAVEGLOBAL))
				{
					if (!(((dd->type&~DEF_SAVEGLOBAL) == ev_vector && (qcc_globals[h].type&~DEF_SAVEGLOBAL) == ev_float) ||
						((dd->type&~DEF_SAVEGLOBAL) == ev_struct || (dd->type&~DEF_SAVEGLOBAL) == ev_union)))
						QCC_PR_Warning(0, NULL, 0, "Mismatched union global types (%s and %s)", strings+dd->s_name, strings+qcc_globals[h].s_name);
				}
				//remove the saveglobal flag on the duplicate globals.
				qcc_globals[h].type &= ~DEF_SAVEGLOBAL;
			}
		}
	}
	for (i = 1; i < numfielddefs; i++)
	{
		dd = &fields[i];

		if (dd->type == ev_vector || dd->type == ev_struct || dd->type == ev_union)	//just ignore vectors, structs, and unions.
			continue;

		for (h = 1; h < numfielddefs; h++)
		{
			if (i == h)
				continue;
			if (dd->ofs == fields[h].ofs)
			{
				if (dd->type != fields[h].type)
				{
					if (fields[h].type != ev_vector && fields[h].type != ev_struct && fields[h].type != ev_union)
					{
						QCC_PR_Warning(0, NULL, 0, "Mismatched union field types (%s and %s @%i)", strings+dd->s_name, strings+fields[h].s_name, dd->ofs);
					}
				}
			}
		}
	}

	if (numglobaldefs > MAX_GLOBALS)
		QCC_Error(ERR_TOOMANYGLOBALS, "Too many globals - %i\nAdd \"MAX_GLOBALS\" \"%i\" to qcc.cfg", numglobaldefs, (numglobaldefs+32768)&~32767);


	dupewarncount = 0;
	for (i = 0; i < nummodels; i++)
	{
		if (!precache_model[i].used)
			dupewarncount+=QCC_PR_Warning(WARN_EXTRAPRECACHE, precache_model[i].filename, precache_model[i].fileline, (dupewarncount>10&&!verbose)?NULL:"Model \"%s\" was precached but not directly used%s", precache_model[i].name, dupewarncount?"":" (annotate the usage with the used_model intrinsic to silence this warning)");
		else if (!precache_model[i].block)
			dupewarncount+=QCC_PR_Warning(WARN_NOTPRECACHED, precache_model[i].filename, precache_model[i].fileline, (dupewarncount>10&&!verbose)?NULL:"Model \"%s\" was used but not directly precached", precache_model[i].name);
	}

	for (i = 0; i < numsounds; i++)
	{
		if (!precache_sound[i].used)
			dupewarncount+=QCC_PR_Warning(WARN_EXTRAPRECACHE, precache_sound[i].filename, precache_sound[i].fileline, (dupewarncount>10&&!verbose)?NULL:"Sound \"%s\" was precached but not directly used", precache_sound[i].name, dupewarncount?"":" (annotate the usage with the used_sound intrinsic to silence this warning)");
		else if (!precache_sound[i].block)
			dupewarncount+=QCC_PR_Warning(WARN_NOTPRECACHED, precache_sound[i].filename, precache_sound[i].fileline, (dupewarncount>10&&!verbose)?NULL:"Sound \"%s\" was used but not directly precached", precache_sound[i].name);
	}

	if (dupewarncount > 10 && !verbose)
		QCC_PR_Note(WARN_NOTREFERENCED, NULL, 0, "suppressed %i more warnings about precaches.", dupewarncount-10);

//PrintStrings ();
//PrintFunctions ();
//PrintFields ();
//PrintGlobals ();
strofs = (strofs+3)&~3;

	if (verbose)
	{
		printf ("%6i strofs (of %i)\n", strofs, MAX_STRINGS);
		printf ("%6i numstatements (of %i)\n", numstatements, MAX_STATEMENTS);
		printf ("%6i numfunctions (of %i)\n", numfunctions, MAX_FUNCTIONS);
		printf ("%6i numglobaldefs (of %i)\n", numglobaldefs, MAX_GLOBALS);
		printf ("%6i numfielddefs (%i unique) (of %i)\n", numfielddefs, pr.size_fields, MAX_FIELDS);
		printf ("%6i numpr_globals (of %i)\n", numpr_globals, MAX_REGS);
		printf ("%6i nummodels\n", nummodels);
		printf ("%6i numsounds\n", numsounds);
	}

	if (!*destfile)
		strcpy(destfile, "progs.dat");
	if (verbose)
		printf("Writing %s\n", destfile);
	h = SafeOpenWrite (destfile, 2*1024*1024);
	SafeWrite (h, &progs, sizeof(progs));
	SafeWrite (h, "\r\n\r\n", 4);
	SafeWrite (h, QCC_copyright, strlen(QCC_copyright)+1);
	SafeWrite (h, "\r\n\r\n", 4);
	while(SafeSeek (h, 0, SEEK_CUR) & 3)//this is a lame way to do it
	{
		SafeWrite (h, "\0", 1);
	}

	progs.ofs_strings = SafeSeek (h, 0, SEEK_CUR);
	progs.numstrings = strofs;

	if (progs.blockscompressed&16)
	{
		SafeWrite (h, &len, sizeof(int));	//save for later
		len = QC_encode(progfuncs, strofs*sizeof(char), 2, (char *)strings, h);	//write
		i = SafeSeek (h, 0, SEEK_CUR);
		SafeSeek(h, progs.ofs_strings, SEEK_SET);//seek back
		len = PRLittleLong(len);
		SafeWrite (h, &len, sizeof(int));	//write size.
		SafeSeek(h, i, SEEK_SET);
	}
	else
		SafeWrite (h, strings, strofs);

	progs.ofs_statements = SafeSeek (h, 0, SEEK_CUR);
	progs.numstatements = numstatements;

	if (qcc_targetformat == QCF_HEXEN2 || qcc_targetformat == QCF_FTEH2)
	{
		for (i=0 ; i<numstatements ; i++)
		{
			if (statements[i].op >= OP_CALL1 && statements[i].op <= OP_CALL8)
				QCC_Error(ERR_BADTARGETSWITCH, "Target switching produced incompatible instructions");
			else if (statements[i].op >= OP_CALL1H && statements[i].op <= OP_CALL8H)
				statements[i].op = statements[i].op - OP_CALL1H + OP_CALL1;
		}
	}

	if (!opt_filenames)
	{
		statement_linenums = qccHunkAlloc(sizeof(statement_linenums) * numstatements);
		for (i = 0; i < numstatements; i++)
			statement_linenums[i] = statements[i].linenum;
	}
	else
		statement_linenums = NULL;


	switch(outputsttype)
	{
	case PST_KKQWSV:
	case PST_FTE32:
		{
			QCC_dstatement32_t *statements32 = qccHunkAlloc(sizeof(*statements32) * numstatements);
			for (i=0 ; i<numstatements ; i++)
			{
				statements32[i].op = PRLittleLong(statements[i].op);
				statements32[i].a = PRLittleLong((statements[i].a.sym?statements[i].a.sym->ofs:0) + statements[i].a.ofs);
				statements32[i].b = PRLittleLong((statements[i].b.sym?statements[i].b.sym->ofs:0) + statements[i].b.ofs);
				statements32[i].c = PRLittleLong((statements[i].c.sym?statements[i].c.sym->ofs:0) + statements[i].c.ofs);

#ifdef DEBUG_DUMP
				printf("code: %s:%i: @%i %s %i %i %i\n", "???", statements[i].linenum, i, pr_opcodes[statements[i].op].name, statements32[i].a, statements32[i].b, statements32[i].c);
#endif
			}

			if (progs.blockscompressed&1)
			{
				SafeWrite (h, &len, sizeof(int));	//save for later
				len = QC_encode(progfuncs, numstatements*sizeof(QCC_dstatement32_t), 2, (char *)statements, h);	//write
				i = SafeSeek (h, 0, SEEK_CUR);
				SafeSeek(h, progs.ofs_statements, SEEK_SET);//seek back
				len = PRLittleLong(len);
				SafeWrite (h, &len, sizeof(int));	//write size.
				SafeSeek(h, i, SEEK_SET);
			}
			else
				SafeWrite (h, statements32, numstatements*sizeof(QCC_dstatement32_t));
		}
		break;
	case PST_QTEST:
		{
			qtest_statement_t *qtst = qccHunkAlloc(sizeof(*qtst) * numstatements);
			for (i=0 ; i<numstatements ; i++) // scale down from 16-byte internal to 12-byte qtest
			{
				unsigned int a = (statements[i].a.sym?statements[i].a.sym->ofs:0) + statements[i].a.ofs;
				unsigned int b = (statements[i].b.sym?statements[i].b.sym->ofs:0) + statements[i].b.ofs;
				unsigned int c = (statements[i].c.sym?statements[i].c.sym->ofs:0) + statements[i].c.ofs;

				qtst[i].line = statements[i].linenum;
				qtst[i].op = PRLittleShort((unsigned short)statements[i].op);
				if (a < 0)
					qtst[i].a = PRLittleShort((short)a);
				else
					qtst[i].a = (unsigned short)PRLittleShort((unsigned short)a);
				if (b < 0)
					qtst[i].b = PRLittleShort((short)b);
				else
					qtst[i].b = (unsigned short)PRLittleShort((unsigned short)b);
				if (c < 0)
					qtst[i].c = PRLittleShort((short)c);
				else
					qtst[i].c = (unsigned short)PRLittleShort((unsigned short)c);
			}

			// no compression
			SafeWrite (h, qtst, numstatements*sizeof(qtest_statement_t));
		}
		break;
	case PST_DEFAULT:
		{
			QCC_dstatement16_t *statements16 = qccHunkAlloc(sizeof(*statements16) * numstatements);
#ifdef DISASM
			int start, end;
			for (i = 1; i < numfunctions; i++)
			{
				if (!strcmp(functions[i].name, DISASM))
				{
					start = functions[i].code;
					end = functions[i+1].code;
				}
			}
#endif
			for (i=0 ; i<numstatements ; i++)	//resize as we go - scaling down
			{
				unsigned int a = (statements[i].a.sym?statements[i].a.sym->ofs:0) + statements[i].a.ofs;
				unsigned int b = (statements[i].b.sym?statements[i].b.sym->ofs:0) + statements[i].b.ofs;
				unsigned int c = (statements[i].c.sym?statements[i].c.sym->ofs:0) + statements[i].c.ofs;
				statements16[i].op = PRLittleShort((unsigned short)statements[i].op);

#if defined(DISASM) && !defined(DEBUG_DUMP)
				if (i >= start && i < end)
#endif
#if defined(DEBUG_DUMP) || defined(DISASM)
				{
					char *astr = statements[i].a.sym?statements[i].a.sym->name:"";
					char *bstr = statements[i].b.sym?statements[i].b.sym->name:"";
					char *cstr = statements[i].c.sym?statements[i].c.sym->name:"";
					printf("code: %s:%i: @%i\t%s\t%i\t%i\t%i\t(%s", "???", statements[i].linenum, i, pr_opcodes[statements[i].op].opname, a, b, c, QCC_VarAtOffset(statements[i].a, 1));
					printf(" %s", QCC_VarAtOffset(statements[i].b, 1));
					printf(" %s)\n", QCC_VarAtOffset(statements[i].c, 1));
				}
#endif
#ifdef _DEBUG
				if (((signed)a >= (signed)numpr_globals && statements[i].a.sym) || ((signed)b >= (signed)numpr_globals && statements[i].b.sym) || ((signed)c >= (signed)numpr_globals && statements[i].c.sym))
					printf("invalid offset on %s instruction\n", pr_opcodes[statements[i].op].opname);
#endif
				if (a < 0)
					statements16[i].a = PRLittleShort(a);
				else
					statements16[i].a = (unsigned short)PRLittleShort(a);
				if (b < 0)
					statements16[i].b = PRLittleShort((short)b);
				else
					statements16[i].b = (unsigned short)PRLittleShort((unsigned short)b);
				if (c < 0)
					statements16[i].c = PRLittleShort((short)c);
				else
					statements16[i].c = (unsigned short)PRLittleShort((unsigned short)c);
			}

			if (progs.blockscompressed&1)
			{
				SafeWrite (h, &len, sizeof(int));	//save for later
				len = QC_encode(progfuncs, numstatements*sizeof(QCC_dstatement16_t), 2, (char *)statements16, h);	//write
				i = SafeSeek (h, 0, SEEK_CUR);
				SafeSeek(h, progs.ofs_statements, SEEK_SET);//seek back
				len = PRLittleLong(len);
				SafeWrite (h, &len, sizeof(int));	//write size.
				SafeSeek(h, i, SEEK_SET);
			}
			else
				SafeWrite (h, statements16, numstatements*sizeof(QCC_dstatement16_t));
		}
		break;
	default:
		Sys_Error("structtype error");
	}

	progs.ofs_functions = SafeSeek (h, 0, SEEK_CUR);
	progs.numfunctions = numfunctions;
	if (progs.blockscompressed&8)
	{
		SafeWrite (h, &len, sizeof(int));	//save for later
		len = QC_encode(progfuncs, funcdatasize, 2, (char *)funcdata, h);	//write
		i = SafeSeek (h, 0, SEEK_CUR);
		SafeSeek(h, progs.ofs_functions, SEEK_SET);//seek back
		len = PRLittleLong(len);
		SafeWrite (h, &len, sizeof(int));	//write size.
		SafeSeek(h, i, SEEK_SET);
	}
	else
		SafeWrite (h, funcdata, funcdatasize);

	if (verbose >= 2)
		QCC_PrintFiles();

	if (verbose >= 2)
		QCC_PrintFields();

	if (verbose >= 2)
		QCC_PrintAutoCvars();

	switch(outputsttype)
	{
	case PST_QTEST:
		// qtest needs a struct remap but should be able to get away with a simple swap here
		for (i=0 ; i<numglobaldefs ; i++)
		{
			qtest_def_t qtdef = ((qtest_def_t *)qcc_globals)[i];
			qcc_globals[i].type = qtdef.type;
			qcc_globals[i].ofs = qtdef.ofs;
			qcc_globals[i].s_name = qtdef.s_name;
		}
		for (i=0 ; i<numfielddefs ; i++)
		{
			qtest_def_t qtdef = ((qtest_def_t *)fields)[i];
			fields[i].type = qtdef.type;
			fields[i].ofs = qtdef.ofs;
			fields[i].s_name = qtdef.s_name;
		}
		// passthrough.. reuse FTE32 code
	case PST_FTE32:
		progs.ofs_globaldefs = SafeSeek (h, 0, SEEK_CUR);
		progs.numglobaldefs = numglobaldefs;
		for (i=0 ; i<numglobaldefs ; i++)
		{
			qcc_globals[i].type = PRLittleLong/*PRLittleShort*/ (qcc_globals[i].type);
			qcc_globals[i].ofs = PRLittleLong/*PRLittleShort*/ (qcc_globals[i].ofs);
			qcc_globals[i].s_name = PRLittleLong (qcc_globals[i].s_name);
		}

		if (progs.blockscompressed&2)
		{
			SafeWrite (h, &len, sizeof(int));	//save for later
			len = QC_encode(progfuncs, numglobaldefs*sizeof(QCC_ddef_t), 2, (char *)qcc_globals, h);	//write
			i = SafeSeek (h, 0, SEEK_CUR);
			SafeSeek(h, progs.ofs_globaldefs, SEEK_SET);//seek back
			len = PRLittleLong(len);
			SafeWrite (h, &len, sizeof(int));	//write size.
			SafeSeek(h, i, SEEK_SET);
		}
		else
			SafeWrite (h, qcc_globals, numglobaldefs*sizeof(QCC_ddef_t));

		progs.ofs_fielddefs = SafeSeek (h, 0, SEEK_CUR);
		progs.numfielddefs = numfielddefs;

		for (i=0 ; i<numfielddefs ; i++)
		{
			fields[i].type = PRLittleLong/*PRLittleShort*/ (fields[i].type);
			fields[i].ofs = PRLittleLong/*PRLittleShort*/ (fields[i].ofs);
			fields[i].s_name = PRLittleLong (fields[i].s_name);
		}

		if (progs.blockscompressed&4)
		{
			SafeWrite (h, &len, sizeof(int));	//save for later
			len = QC_encode(progfuncs, numfielddefs*sizeof(QCC_ddef_t), 2, (char *)fields, h);	//write
			i = SafeSeek (h, 0, SEEK_CUR);
			SafeSeek(h, progs.ofs_fielddefs, SEEK_SET);//seek back
			len = PRLittleLong(len);
			SafeWrite (h, &len, sizeof(int));	//write size.
			SafeSeek(h, i, SEEK_SET);
		}
		else
			SafeWrite (h, fields, numfielddefs*sizeof(QCC_ddef_t));
		break;
	case PST_KKQWSV:
	case PST_DEFAULT:
#define qcc_globals16 ((QCC_ddef16_t*)qcc_globals)
#define fields16 ((QCC_ddef16_t*)fields)
		progs.ofs_globaldefs = SafeSeek (h, 0, SEEK_CUR);
		progs.numglobaldefs = numglobaldefs;
		for (i=0 ; i<numglobaldefs ; i++)
		{
			qcc_globals16[i].type = (unsigned short)PRLittleShort ((unsigned short)qcc_globals[i].type);
			qcc_globals16[i].ofs = (unsigned short)PRLittleShort ((unsigned short)qcc_globals[i].ofs);
			qcc_globals16[i].s_name = PRLittleLong (qcc_globals[i].s_name);
		}

		if (progs.blockscompressed&2)
		{
			SafeWrite (h, &len, sizeof(int));	//save for later
			len = QC_encode(progfuncs, numglobaldefs*sizeof(QCC_ddef16_t), 2, (char *)qcc_globals16, h);	//write
			i = SafeSeek (h, 0, SEEK_CUR);
			SafeSeek(h, progs.ofs_globaldefs, SEEK_SET);//seek back
			len = PRLittleLong(len);
			SafeWrite (h, &len, sizeof(int));	//write size.
			SafeSeek(h, i, SEEK_SET);
		}
		else
			SafeWrite (h, qcc_globals16, numglobaldefs*sizeof(QCC_ddef16_t));

		progs.ofs_fielddefs = SafeSeek (h, 0, SEEK_CUR);
		progs.numfielddefs = numfielddefs;

		for (i=0 ; i<numfielddefs ; i++)
		{
			fields16[i].type = (unsigned short)PRLittleShort ((unsigned short)fields[i].type);
			if (fields[i].ofs > 0xffff)
			{
				printf("Offset for field %s overflowed 16 bits.\n", strings+fields[i].s_name);
				fields16[i].ofs = 0;
			}
			else
				fields16[i].ofs = (unsigned short)PRLittleShort ((unsigned short)fields[i].ofs);
			fields16[i].s_name = PRLittleLong (fields[i].s_name);
		}

		if (progs.blockscompressed&4)
		{
			SafeWrite (h, &len, sizeof(int));	//save for later
			len = QC_encode(progfuncs, numfielddefs*sizeof(QCC_ddef16_t), 2, (char *)fields16, h);	//write
			i = SafeSeek (h, 0, SEEK_CUR);
			SafeSeek(h, progs.ofs_fielddefs, SEEK_SET);//seek back
			len = PRLittleLong(len);
			SafeWrite (h, &len, sizeof(int));	//write size.
			SafeSeek(h, i, SEEK_SET);
		}
		else
			SafeWrite (h, fields16, numfielddefs*sizeof(QCC_ddef16_t));
		break;
	default:
		Sys_Error("structtype error");
	}

	progs.ofs_globals = SafeSeek (h, 0, SEEK_CUR);
	progs.numglobals = numpr_globals;

	for (i=0 ; (unsigned)i<numpr_globals ; i++)
		((int *)qcc_pr_globals)[i] = PRLittleLong (((int *)qcc_pr_globals)[i]);

	if (progs.blockscompressed&32)
	{
		SafeWrite (h, &len, sizeof(int));	//save for later
		len = QC_encode(progfuncs, numpr_globals*4, 2, (char *)qcc_pr_globals, h);	//write
		i = SafeSeek (h, 0, SEEK_CUR);
		SafeSeek(h, progs.ofs_globals, SEEK_SET);//seek back
		len = PRLittleLong(len);
		SafeWrite (h, &len, sizeof(int));	//write size.
		SafeSeek(h, i, SEEK_SET);
	}
	else
		SafeWrite (h, qcc_pr_globals, numpr_globals*4);

/*
	if (types)
	for (i=0 ; i<numtypeinfos ; i++)
	{
		if (qcc_typeinfo[i].aux_type)
			qcc_typeinfo[i].aux_type = (QCC_type_t*)(qcc_typeinfo[i].aux_type - qcc_typeinfo);
		if (qcc_typeinfo[i].next)
			qcc_typeinfo[i].next = (QCC_type_t*)(qcc_typeinfo[i].next - qcc_typeinfo);
		qcc_typeinfo[i].name = (char*)QCC_CopyDupBackString(qcc_typeinfo[i].name);
	}
*/
	progs.ofsfiles = 0;
	progs.ofslinenums = 0;
	progs.secondaryversion = 0;
	progs.ofsbodylessfuncs = 0;
	progs.numbodylessfuncs = 0;
	progs.ofs_types = 0;
	progs.numtypes = 0;

	
	progs.ofsbodylessfuncs = SafeSeek (h, 0, SEEK_CUR);
	progs.numbodylessfuncs = WriteBodylessFuncs(h);

	switch(qcc_targetformat)
	{
	case QCF_QTEST:
		progs.version = PROG_QTESTVERSION;
		progs.ofsfiles = WriteSourceFiles(qcc_sourcefile, h, debugtarget, false);
		break;
	case QCF_KK7:
		progs.version = PROG_KKQWSVVERSION;
		progs.ofsfiles = WriteSourceFiles(qcc_sourcefile, h, debugtarget, false);
		break;
	default:
	case QCF_STANDARD:
	case QCF_HEXEN2:	//urgh
		progs.version = PROG_VERSION;
		progs.ofsfiles = WriteSourceFiles(qcc_sourcefile, h, debugtarget, false);
		break;
	case QCF_DARKPLACES:
	case QCF_FTE:
	case QCF_FTEH2:
	case QCF_FTEDEBUG:
		progs.version = PROG_EXTENDEDVERSION;

		if (outputsttype == PST_FTE32)
			progs.secondaryversion = PROG_SECONDARYVERSION32;
		else
			progs.secondaryversion = PROG_SECONDARYVERSION16;

		if (debugtarget && statement_linenums)
		{
			progs.ofslinenums = SafeSeek (h, 0, SEEK_CUR);
			if (progs.blockscompressed&64)
			{
				SafeWrite (h, &len, sizeof(int));	//save for later
				len = QC_encode(progfuncs, numstatements*sizeof(int), 2, (char *)statement_linenums, h);	//write
				i = SafeSeek (h, 0, SEEK_CUR);
				SafeSeek(h, progs.ofslinenums, SEEK_SET);//seek back
				len = PRLittleLong(len);
				SafeWrite (h, &len, sizeof(int));	//write size.
				SafeSeek(h, i, SEEK_SET);
			}
			else
				SafeWrite (h, statement_linenums, numstatements*sizeof(int));
			statement_linenums = NULL;
		}
		else
			progs.ofslinenums = 0;

		if (types)
		{
			progs.ofs_types = SafeSeek (h, 0, SEEK_CUR);
			if (progs.blockscompressed&128)
			{
				SafeWrite (h, &len, sizeof(int));	//save for later
				len = QC_encode(progfuncs, sizeof(QCC_type_t)*numtypeinfos, 2, (char *)qcc_typeinfo, h);	//write
				i = SafeSeek (h, 0, SEEK_CUR);
				SafeSeek(h, progs.ofs_types, SEEK_SET);//seek back#
				len = PRLittleLong(len);
				SafeWrite (h, &len, sizeof(int));	//write size.
				SafeSeek(h, i, SEEK_SET);
			}
			else
				SafeWrite (h, qcc_typeinfo, sizeof(QCC_type_t)*numtypeinfos);
			progs.numtypes = numtypeinfos;
		}
		else
		{
			progs.ofs_types = 0;
			progs.numtypes = 0;
		}

		progs.ofsfiles = WriteSourceFiles(qcc_sourcefile, h, debugtarget, true);
		break;
	}
	qcc_sourcefile = NULL;

	if (progs.version != PROG_EXTENDEDVERSION && progs.numbodylessfuncs)
		printf ("WARNING: progs format cannot handle extern functions\n");


	if (verbose)
		printf ("%6i TOTAL SIZE\n", (int)SafeSeek (h, 0, SEEK_CUR));

	progs.entityfields = pr.size_fields;

	progs.crc = crc;

	def = QCC_PR_GetDef(NULL, "progs", NULL, false, 0, 0);	//this is for qccx support
	if (def && (def->type->type == ev_entity || (def->type->type == ev_accessor && def->type->parentclass->type == ev_entity)))
	{
		int size = SafeSeek (h, 0, SEEK_CUR);
		size += 1;	//the engine will add a null terminator
		size = (size+15)&(~15);	//and will allocate it on the hunk with 16-byte alignment

		//this global receives the offset from world to the start of the progs def _IN VANILLA QUAKE_.
		//this is a negative index due to allocation ordering
		//this will NOT work in FTE, DP, QuakeForge due to entity indexes. Various other engines will likely mess up too, if they change the allocation order or sizes etc. 64bit is screwed.
		if (progs.blockscompressed&32)
			printf("unable to write value for 'entity progs'\n");	//would not work anyway
		else
		{
			QCC_PR_Warning(WARN_DENORMAL, def->filen, def->s_line, "'entity progs' is non-portable and will not work across engines nor cpus.");

			if (def->initialized)
				i = PRLittleLong(qcc_pr_globals[def->ofs]._int);
			else
			{
				if (verbose)
					printf("qccx hack - 'entity progs' uninitialised. Assuming 112.\n");
				i = 112;	//match qccx.
			}
			i = -(size + i);
			i = PRLittleLong(i);
			SafeSeek (h, progs.ofs_globals + 4 * def->ofs, SEEK_SET);
			SafeWrite (h, &i, 4);
		}
	}

	// qbyte swap the header and write it out
	for (i=0 ; i<sizeof(progs)/4 ; i++)
		((int *)&progs)[i] = PRLittleLong ( ((int *)&progs)[i] );
	SafeSeek (h, 0, SEEK_SET);
	SafeWrite (h, &progs, sizeof(progs));


	if (!SafeClose (h))
	{
		printf("Error while writing output %s\n", destfile);
		return false;
	}





	switch(qcc_targetformat)
	{
	case QCF_QTEST:
		printf("Compile finished: %s (qtest format)\n", destfile);
		break;
	case QCF_KK7:
		printf("Compile finished: %s (kk7 format)\n", destfile);
		break;
	case QCF_STANDARD:
		printf("Compile finished: %s (id format)\n", destfile);
		break;
	case QCF_HEXEN2:
		printf("Compile finished: %s (hexen2 format)\n", destfile);
		break;
	case QCF_DARKPLACES:
		printf("Compile finished: %s (dp format)\n", destfile);
		break;
	case QCF_FTE:
		printf("Compile finished: %s (fte format)\n", destfile);
		break;
	case QCF_FTEH2:
		printf("Compile finished: %s (fteh2 format)\n", destfile);
		break;
	case QCF_FTEDEBUG:
		printf("Compile finished: %s (ftedbg format)\n", destfile);
		break;
	default:
		printf("Compile finished: %s\n", destfile);
		break;
	}

	if (statement_linenums)
	{
		unsigned int lnotype = *(unsigned int*)"LNOF";
		unsigned int version = 1;
		pbool gz = false;
		while(1)
		{
			char *ext;
			ext = strrchr(destfile, '.');
			if (strchr(ext, '/') || strchr(ext, '\\'))
				break;
			if (!stricmp(ext, ".gz"))
			{
				*ext = 0;
				gz = true;
				continue;
			}
			*ext = 0;
			break;
		}
		if (strlen(destfile) < sizeof(destfile)-(4+3))
		{
			strcat(destfile, ".lno");
			if (gz)
				strcat(destfile, ".gz");
			if (verbose)
				printf("Writing %s for debugging\n", destfile);
			h = SafeOpenWrite (destfile, 2*1024*1024);
			SafeWrite (h, &lnotype, sizeof(int));
			SafeWrite (h, &version, sizeof(int));
			SafeWrite (h, &numglobaldefs, sizeof(int));
			SafeWrite (h, &numpr_globals, sizeof(int));
			SafeWrite (h, &numfielddefs, sizeof(int));
			SafeWrite (h, &numstatements, sizeof(int));
			SafeWrite (h, statement_linenums, numstatements*sizeof(int));
			SafeClose (h);
		}
	}

	return true;
}

/*
#merge "oldprogs"
wrap void() worldspawn =
{
	print("hello world\n");
	prior();
};

Progs merging is done by loading in an existing progs.dat and essentially appending new stuff on the end.
The resulting output should be the same, other than wraps (which replaces the previous function global with the new one).
*/
static void QCC_MergeStrings(char *in, unsigned int num)
{
	memcpy(strings, in, num);
	strofs = num;
}
int QCC_MergeValidateString(int str)
{
	if (str < 0 || str >= strofs)
		str = 0;
	return str;
}
static void QCC_MergeFunctions(dfunction_t *in, unsigned int num)
{
	numfunctions = 0;
	while(num --> 0)
	{
		if (in->first_statement <= 0)
		{
			functions[numfunctions].builtin = -in->first_statement;
			functions[numfunctions].code = -1;
		}
		else
		{
			functions[numfunctions].builtin = 0;
			functions[numfunctions].code = in->first_statement;
		}
		functions[numfunctions].s_filed = QCC_MergeValidateString(in->s_file);
		functions[numfunctions].filen = strings+functions[numfunctions].s_filed;
		functions[numfunctions].line = 0;
		functions[numfunctions].name = strings+QCC_MergeValidateString(in->s_name);
		functions[numfunctions].parentscope = NULL;
		functions[numfunctions].type = NULL;
		functions[numfunctions].def = NULL;
		functions[numfunctions].firstlocal = NULL;
		functions[numfunctions].privatelocals = true;
		functions[numfunctions].merged = in;
		numfunctions++;
		in++;
	}
}
static void QCC_MergeStatements16(dstatement16_t *in, unsigned int num)
{
	QCC_statement_t *out = statements;
	numstatements = num;
	for (; num --> 0; out++, in++)
	{
		out->op = in->op;
		out->a.sym = NULL;
		out->a.cast = NULL;
		out->a.ofs = in->a;
		out->b.sym = NULL;
		out->b.cast = NULL;
		out->b.ofs = in->b;
		out->c.sym = NULL;
		out->c.cast = NULL;
		out->c.ofs = in->c;
		out->linenum = 0;
	}
	
	out->op = OP_DONE;
	out->a.ofs = 0;
	out->b.ofs = 0;
	out->c.ofs = 0;
	out->linenum = 0;
	numstatements++;
}
QCC_def_t *QCC_PR_DummyDef(QCC_type_t *type, char *name, QCC_function_t *scope, int arraysize, QCC_def_t *rootsymbol, unsigned int ofs, int referable, unsigned int flags);
static etype_t QCC_MergeFindFieldType(unsigned int ofs, const char *fldname, ddef16_t *fields, size_t numfields)
{
	size_t i;
	etype_t best = ev_void;
	for (i = 0; i < numfields; i++)
	{
		if (fields[i].ofs == ofs)
		{	//sometimes we have field unions. go for the exact name match if we can so we don't get confused over vectors/floats. otherwise just go with the first (and hope they're correctly ordered)
			char *name = strings+QCC_MergeValidateString(fields[i].s_name);
			if (!strcmp(name, fldname))
				return fields[i].type;
			if (best == ev_void)
				best = fields[i].type;
		}
	}
	return best;
}
static void QCC_MergeUnstrip(dfunction_t *in, unsigned int num)
{
	size_t i;
	char *name;
	QCC_def_t *def;

	//functions may have been stripped. this results in an annoying lack of errors, and will likely confuse function wrapping...
	//generate a new def for each function, if it doesn't already exist.
	//these are probably going to be wasteful dupes, but they'll just get stripped again if they're still not used.
	for (i = 0; i < num; i++)
	{
		if (!in[i].s_name)
			continue;

		name = strings+QCC_MergeValidateString(in[i].s_name);

		def = QCC_PR_GetDef(NULL, name, NULL, false, 0, GDF_BASICTYPE);
		if (!def)
		{
			def = QCC_PR_GetDef(type_function, name, NULL, true, 0, GDF_BASICTYPE);
			def->symboldata[def->ofs].function = i;
			def->initialized = true;
			def->referenced = true;
			def->assumedtype = true;
		}
		QCC_FreeDef(def);
	}
}
QCC_type_t *QCC_PR_FieldType (QCC_type_t *pointsto);
static void QCC_MergeGlobalDefs16(ddef16_t *in, size_t num, void *values, size_t defscount, ddef16_t *fields, size_t numfields)
{
	QCC_def_t *root, *def;
	QCC_type_t *type;
	etype_t evt;

	char *name;
	unsigned int flags;
	pbool referrable;

	numpr_globals = 0;	//that root object will replace the normal reserved globals.
	root = QCC_PR_GetDef(type_void, "", NULL, true, 0, GDF_USED);
	root->symboldata = values;
	root->symbolsize = defscount;

	for (; num --> 0; in++)
	{
		name = strings+QCC_MergeValidateString(in->s_name);

		flags = GDF_USED;
		if (in->type & DEF_SAVEGLOBAL)
			flags |= GDF_SAVED;

		evt = in->type&~DEF_SAVEGLOBAL;
		if (evt == ev_field)
			evt = QCC_MergeFindFieldType(root->symboldata[in->ofs]._int, name, fields, numfields);
		switch(evt)
		{
		case ev_void:
			type = type_void;
			break;
		case ev_vector:
			type = type_vector;
			break;
		case ev_float:
			type = type_float;
			break;
		case ev_string:
			type = type_string;
			break;
		case ev_entity:
			type = type_entity;
			break;
		case ev_integer:
			type = type_integer;
			break;
		case ev_function:
			type = type_function;
			break;
		default:
			type = type_variant;
			break;
		}
		if ((in->type&~DEF_SAVEGLOBAL) == ev_field)
		{
			type = QCC_PR_FieldType(type);
			flags |= GDF_CONST;
		}

		referrable = true;	//fixme: disable if this appears to be within a function's local storage
		
		def = QCC_PR_DummyDef(type, name, NULL, 0, root, in->ofs, referrable, flags);
		def->initialized = 1;
		def->referenced = true;
		def->assumedtype = true;

		if (evt == ev_vector)
		{
			int j = 3;
			if ((in->type&~DEF_SAVEGLOBAL) == ev_field)
			{
				for (j = 0; j < 3; j++)
				{
					if (in[j+1].ofs == in->ofs+j && (in[j+1].type&~DEF_SAVEGLOBAL) == ev_field && QCC_MergeFindFieldType(root->symboldata[in[j+1].ofs]._int, strings+QCC_MergeValidateString(in[j+1].s_name), fields, numfields) == ev_float)
						continue;
					break;
				}
			}
			else
			{
				for (j = 0; j < 3; j++)
				{
					if (in[j+1].ofs == in->ofs+j && (in[j+1].type&~DEF_SAVEGLOBAL) == ev_float)
						continue;
					break;
				}
			}
			in += j;
			num -= j;
		}
	}
	QCC_FreeDef(root);
}

/*load a progs into the current compile state.*/
void QCC_ImportProgs(const char *filename)
{
	int flen;
	dprograms_t *prog;

	//these keywords are implicitly enabled by #merge
	keyword_weak = true;
	keyword_wrap = true;

//	if (strofs != 0)		//could be fixed with relocs
//		QCC_Error(ERR_BADEXTENSION, "#merge used too late. It must be used before any other definitions.");
	if (numstatements != 1)	//should be easy to deal with.
		QCC_Error(ERR_BADEXTENSION, "#merge used too late. It must be used before any other definitions.");
	if (numfunctions != 1)	//could be fixed with relocs
		QCC_Error(ERR_BADEXTENSION, "#merge used too late. It must be used before any other definitions.");
	if (numglobaldefs != 1)	//could be fixed by inserting it properly. any already-defined defs must have their parentdef changed to union them with imported ones.
		QCC_Error(ERR_BADEXTENSION, "#merge used too late. It must be used before any other definitions (globals).");
	if (numfielddefs != 1)	//could be fixed with relocs
		QCC_Error(ERR_BADEXTENSION, "#merge used too late. It must be used before any other definitions (fields).");
	if (numpr_globals != RESERVED_OFS)	//not normally changed until after compiling
		QCC_Error(ERR_BADEXTENSION, "#merge used too late. It must be used before any other definitions (regs).");

	flen = externs->FileSize(filename);
	if (flen < 0)
	{
		QCC_Error(ERR_COULDNTOPENFILE, "Couldn't open file %s", filename);
		return;
	}

	printf ("\nnote: The #merge feature is still experimental\n\n");
	//FIXME: find overlapped locals. strip them. merge with new ones.
	//FIXME: find temps. strip them. you get the idea.
	//FIXME: find immediates. set up hash tables for them for reuse. HAH!

	prog = qccHunkAlloc(flen);

	externs->ReadFile(filename, prog, flen, NULL);

	if (prog->version == 7 && prog->secondaryversion == PROG_SECONDARYVERSION16 && !prog->blockscompressed && !prog->numtypes)
		;
	else if (prog->version == 7 && prog->secondaryversion == PROG_SECONDARYVERSION32 && !prog->blockscompressed && !prog->numtypes)
		;
	else if (prog->version != 6)
	{
		QCC_Error(ERR_COULDNTOPENFILE, "Unsupported version: %s", filename);
		return;
	}

	QCC_MergeStrings(((char*)prog+prog->ofs_strings), prog->numstrings);
	QCC_MergeFunctions((dfunction_t*)((char*)prog+prog->ofs_functions), prog->numfunctions);
	pr.size_fields = prog->entityfields;
	if (prog->version == 7 && prog->secondaryversion == PROG_SECONDARYVERSION32)
	{
//		QCC_MergeStatements32((dstatement32_t*)((char*)prog+prog->ofs_statements), prog->numstatements);
//		QCC_MergeGlobalDefs32((ddef32_t*)((char*)prog+prog->ofs_globaldefs), prog->numglobaldefs, ((char*)prog)+prog->ofs_globals, prog->numglobals, (ddef16_t*)((char*)prog+prog->ofs_fielddefs), prog->numfielddefs);
		QCC_Error(ERR_COULDNTOPENFILE, "32bit versions not supported: %s", filename);
	}
	else
	{
		QCC_MergeStatements16((dstatement16_t*)((char*)prog+prog->ofs_statements), prog->numstatements);
		QCC_MergeGlobalDefs16((ddef16_t*)((char*)prog+prog->ofs_globaldefs), prog->numglobaldefs, ((char*)prog)+prog->ofs_globals, prog->numglobals, (ddef16_t*)((char*)prog+prog->ofs_fielddefs), prog->numfielddefs);
	}
	QCC_MergeUnstrip((dfunction_t*)((char*)prog+prog->ofs_functions), prog->numfunctions);
}


/*
===============
PR_String

Returns a string suitable for printing (no newlines, max 60 chars length)
===============
*/
char *QCC_PR_String (char *string)
{
	static char buf[80];
	char	*s;

	s = buf;
	*s++ = '"';
	while (string && *string)
	{
		if (s == buf + sizeof(buf) - 2)
			break;
		if (*string == '\n')
		{
			*s++ = '\\';
			*s++ = 'n';
		}
		else if (*string == '"')
		{
			*s++ = '\\';
			*s++ = '"';
		}
		else
			*s++ = *string;
		string++;
		if (s - buf > 60)
		{
			*s++ = '.';
			*s++ = '.';
			*s++ = '.';
			break;
		}
	}
	*s++ = '"';
	*s++ = 0;
	return buf;
}



QCC_def_t	*QCC_PR_DefForFieldOfs (gofs_t ofs)
{
	QCC_def_t	*d;

	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (d->type->type != ev_field)
			continue;
		if (*((unsigned int *)&qcc_pr_globals[d->ofs]) == ofs)
			return d;
	}
	QCC_Error (ERR_NOTDEFINED, "PR_DefForFieldOfs: couldn't find %i",ofs);
	return NULL;
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char *QCC_PR_ValueString (etype_t type, void *val)
{
	static char	line[256];
	QCC_def_t		*def;
	QCC_function_t	*f;

	switch (type)
	{
	case ev_string:
		QC_snprintfz (line, sizeof(line), "%s", QCC_PR_String(strings + *(int *)val));
		break;
	case ev_entity:
		QC_snprintfz (line, sizeof(line), "entity %i", *(int *)val);
		break;
	case ev_function:
		f = functions + *(int *)val;
		if (!f)
			QC_snprintfz (line, sizeof(line), "undefined function");
		else
			QC_snprintfz (line, sizeof(line), "%s()", f->name);
		break;
	case ev_field:
		def = QCC_PR_DefForFieldOfs ( *(int *)val );
		QC_snprintfz (line, sizeof(line), ".%s", def->name);
		break;
	case ev_void:
		QC_snprintfz (line, sizeof(line), "void");
		break;
	case ev_float:
		QC_snprintfz (line, sizeof(line), "%5.1f", *(float *)val);
		break;
	case ev_integer:
		QC_snprintfz (line, sizeof(line), "%i", *(int *)val);
		break;
	case ev_vector:
		QC_snprintfz (line, sizeof(line), "'%5.1f %5.1f %5.1f'", ((float *)val)[0], ((float *)val)[1], ((float *)val)[2]);
		break;
	case ev_pointer:
		QC_snprintfz (line, sizeof(line), "pointer");
		break;
	default:
		QC_snprintfz (line, sizeof(line), "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
/*char *QCC_PR_GlobalStringNoContents (gofs_t ofs)
{
	int		i;
	QCC_def_t	*def;
	void	*val;
	static char	line[128];

	val = (void *)&qcc_pr_globals[ofs];
	def = pr_global_defs[ofs];
	if (!def)
//		Error ("PR_GlobalString: no def for %i", ofs);
		QC_snprintfz (line, sizeof(line), "%i(?""?""?)", ofs);
	else
		QC_snprintfz (line, sizeof(line), "%i(%s)", ofs, def->name);

	i = strlen(line);
	for ( ; i<16 ; i++)
		Q_strlcat (line," ", sizeof(line));
	Q_strlcat (line," ", sizeof(line));

	return line;
}

char *QCC_PR_GlobalString (gofs_t ofs)
{
	char	*s;
	int		i;
	QCC_def_t	*def;
	void	*val;
	static char	line[128];

	val = (void *)&qcc_pr_globals[ofs];
	def = pr_global_defs[ofs];
	if (!def)
		return QCC_PR_GlobalStringNoContents(ofs);
	if (def->initialized && def->type->type != ev_function)
	{
		s = QCC_PR_ValueString (def->type->type, &qcc_pr_globals[ofs]);
		QC_snprintfz (line, sizeof(line), "%i(%s)", ofs, s);
	}
	else
		QC_snprintfz (line, sizeof(line), "%i(%s)", ofs, def->name);

	i = strlen(line);
	for ( ; i<16 ; i++)
		strcat (line," ");
	strcat (line," ");

	return line;
}*/

/*
============
PR_PrintOfs
============
*/
/*void QCC_PR_PrintOfs (gofs_t ofs)
{
	printf ("%s\n",QCC_PR_GlobalString(ofs));
}*/

/*
=================
PR_PrintStatement
=================
*/
/*void QCC_PR_PrintStatement (QCC_dstatement_t *s)
{
	int		i;

	printf ("%4i : %4i : %s ", (int)(s - statements), statement_linenums[s-statements], pr_opcodes[s->op].opname);
	i = strlen(pr_opcodes[s->op].opname);
	for ( ; i<10 ; i++)
		printf (" ");

	if (s->op == OP_IF || s->op == OP_IFNOT)
		printf ("%sbranch %i",QCC_PR_GlobalString(s->a),s->b);
	else if (s->op == OP_GOTO)
	{
		printf ("branch %i",s->a);
	}
	else if ( (unsigned)(s->op - OP_STORE_F) < 6)
	{
		printf ("%s",QCC_PR_GlobalString(s->a));
		printf ("%s", QCC_PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			printf ("%s",QCC_PR_GlobalString(s->a));
		if (s->b)
			printf ("%s",QCC_PR_GlobalString(s->b));
		if (s->c)
			printf ("%s", QCC_PR_GlobalStringNoContents(s->c));
	}
	printf ("\n");
}*/


/*
============
PR_PrintDefs
============
*/
/*void QCC_PR_PrintDefs (void)
{
	QCC_def_t	*d;

	for (d=pr.def_head.next ; d ; d=d->next)
		QCC_PR_PrintOfs (d->ofs);
}*/

QCC_type_t *QCC_PR_NewType (char *name, int basictype, pbool typedefed)
{
	if (numtypeinfos>= maxtypeinfos)
		QCC_Error(ERR_TOOMANYTYPES, "Too many types");
	memset(&qcc_typeinfo[numtypeinfos], 0, sizeof(QCC_type_t));
	qcc_typeinfo[numtypeinfos].type = basictype;
	qcc_typeinfo[numtypeinfos].name = name;
	qcc_typeinfo[numtypeinfos].num_parms = 0;
	qcc_typeinfo[numtypeinfos].params = NULL;
	qcc_typeinfo[numtypeinfos].size = type_size[basictype];
	qcc_typeinfo[numtypeinfos].typedefed = typedefed;


	numtypeinfos++;

	return &qcc_typeinfo[numtypeinfos-1];
}

/*
==============
PR_BeginCompilation

called before compiling a batch of files, clears the pr struct
==============
*/
void	QCC_PR_BeginCompilation (void *memory, int memsize)
{
	extern int recursivefunctiontype;
	int		i;
	char name[16];

	pr.memory = memory;
	pr.max_memory = memsize;

	pr.def_tail = &pr.def_head;
	pr.local_tail = &pr.local_head;

	QCC_PR_ResetErrorScope();
	pr_scope = NULL;

/*	numpr_globals = RESERVED_OFS;

	for (i=0 ; i<RESERVED_OFS ; i++)
		pr_global_defs[i] = &def_void;
*/

	type_void = QCC_PR_NewType("void", ev_void, true);
	type_string = QCC_PR_NewType("string", ev_string, true);
	type_float = QCC_PR_NewType("float", ev_float, true);
	type_vector = QCC_PR_NewType("vector", ev_vector, true);
	type_entity = QCC_PR_NewType("entity", ev_entity, true);
	type_field = QCC_PR_NewType("__field", ev_field, false);
	type_function = QCC_PR_NewType("__function", ev_function, false);
	type_function->aux_type = type_void;
	type_pointer = QCC_PR_NewType("__pointer", ev_pointer, false);
	type_integer = QCC_PR_NewType("__integer", ev_integer, true);
	type_variant = QCC_PR_NewType("variant", ev_variant, true);
	type_variant = QCC_PR_NewType("__variant", ev_variant, true);

	type_floatfield = QCC_PR_NewType("fieldfloat", ev_field, false);
	type_floatfield->aux_type = type_float;
	type_pointer->aux_type = QCC_PR_NewType("pointeraux", ev_float, false);

	type_intpointer = QCC_PR_NewType("__intpointer", ev_pointer, false);
	type_intpointer->aux_type = type_integer;
	type_floatpointer = QCC_PR_NewType("__floatpointer", ev_pointer, false);
	type_floatpointer->aux_type = type_float;
	type_floatfunction = QCC_PR_NewType("__floatfunction", ev_function, false);
	type_floatfunction->aux_type = type_float;

	//type_field->aux_type = type_float;

	type_integer = QCC_PR_NewType("integer", ev_integer, keyword_integer?true:false);
	type_integer = QCC_PR_NewType("int", ev_integer, keyword_integer?true:false);



	if (output_parms)
	{	//this tends to confuse the brains out of decompilers. :)
		numpr_globals = 1;
		QCC_PR_GetDef(type_vector, "RETURN", NULL, true, 0, false)->referenced=true;
		for (i = 0; i < MAX_PARMS; i++)
		{
			QC_snprintfz (name, sizeof(name), "PARM%i", i);
			QCC_PR_GetDef(type_vector, name, NULL, true, 0, false)->referenced=true;
		}
	}
	else
	{
		numpr_globals = RESERVED_OFS;
//		for (i=0 ; i<RESERVED_OFS ; i++)
//			pr_global_defs[i] = NULL;
	}

// link the function type in so state forward declarations match proper type
	pr.types = NULL;
//	type_function->next = NULL;
	pr_error_count = 0;
	pr_warning_count = 0;
	recursivefunctiontype = 0;
}

/*
==============
PR_FinishCompilation

called after all files are compiled to check for errors
Returns false if errors were detected.
==============
*/
int QCC_PR_FinishCompilation (void)
{
	QCC_def_t		*d;
	QCC_type_t		*t;
	int	errors;

	pbool externokay = false;

	errors = false;

	if (pr_error_count)
		return false;

	if (qcc_targetformat == QCF_FTE || qcc_targetformat == QCF_FTEDEBUG || qcc_targetformat == QCF_FTEH2)
		externokay = true;

// check to make sure all functions prototyped have code
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (d->type->type == ev_function && d->constant)// function parms are ok
		{
//			f = G_FUNCTION(d->ofs);
//			if (!f || (!f->code && !f->builtin) )
			if (d->initialized==0)
			{
				if (!strncmp(d->name, "ArrayGet*", 9))
				{
					QCC_PR_EmitArrayGetFunction(d, d->generatedfor, d->name+9);
					pr_scope = NULL;
					continue;
				}
				if (!strncmp(d->name, "ArraySet*", 9))
				{
					QCC_PR_EmitArraySetFunction(d, d->generatedfor, d->name+9);
					pr_scope = NULL;
					continue;
				}
				if (!strncmp(d->name, "spawnfunc_", 10))
				{
					//not all of these will have a class defined, as some will be regular spawn functions, so don't error on that
					t = QCC_TypeForName(d->name+10);
					if (t && t->type == ev_entity)
					{
						QCC_PR_EmitClassFromFunction(d, t);
						pr_scope = NULL;
						continue;
					}
				}
				QCC_PR_Warning(ERR_NOFUNC, d->filen, d->s_line, "function %s has no body",d->name);
				QCC_PR_ParsePrintDef(ERR_NOFUNC, d);
				bodylessfuncs = true;
				errors = true;
			}
			else if (d->initialized==2)
			{
				if (!externokay)
				{
					QCC_PR_Warning(ERR_NOFUNC, d->filen, d->s_line, "extern is not supported with this target format",d->name);
					QCC_PR_ParsePrintDef(ERR_NOFUNC, d);
					errors = true;
				}
				bodylessfuncs = true;
			}
		}
	}
	pr_scope = NULL;

	return !errors;
}

//=============================================================================

// FIXME: byte swap?

// this is a 16 bit, non-reflected CRC using the polynomial 0x1021
// and the initial and final xor values shown below...  in other words, the
// CCITT standard CRC used by XMODEM


#define CRC_INIT_VALUE	0xffff
#define CRC_XOR_VALUE	0x0000

static unsigned short QCC_crctable[256] =
{
	0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
	0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
	0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
	0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
	0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
	0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
	0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
	0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
	0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
	0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
	0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
	0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
	0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
	0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
	0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
	0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
	0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
	0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
	0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
	0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
	0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
	0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
	0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
	0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
	0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
	0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
	0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
	0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
	0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
	0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
	0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
	0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};

void QCC_CRC_Init(unsigned short *crcvalue)
{
	*crcvalue = CRC_INIT_VALUE;
}

void QCC_CRC_ProcessByte(unsigned short *crcvalue, qbyte data)
{
	*crcvalue = ((*crcvalue << 8) ^ QCC_crctable[(*crcvalue >> 8) ^ data]) & 0xffff;
}

unsigned short QCC_CRC_Value(unsigned short crcvalue)
{
	return crcvalue ^ CRC_XOR_VALUE;
}
//=============================================================================


/*
============
PR_WriteProgdefs

Writes the global and entity structures out
Returns a crc of the header, to be stored in the progs file for comparison
at load time.
============
*/
/*
char *Sva(char *msg, ...)
{
	va_list l;
	static char buf[1024];

	va_start(l, msg);
	QC_vsnprintf (buf,sizeof(buf)-1, msg, l);
	va_end(l);

	return buf;
}
*/

#define PROGDEFS_MAX_SIZE 16384
//write (to file buf) and add to the crc
static void Add_WithCRC(char *p, unsigned short *crc, char *file)
{
	char *s;
	int i = strlen(file);
	if (i + strlen(p)+1 >= PROGDEFS_MAX_SIZE)
		return;
	for(s=p;*s;s++,i++)
	{
		QCC_CRC_ProcessByte(crc, *s);
		file[i] = *s;
	}
	file[i]='\0';
}
#define ADD_CRC(p) Add_WithCRC(p, &crc, file)
//#define ADD(p) {char *s;int i = strlen(p);for(s=p;*s;s++,i++){QCC_CRC_ProcessByte(&crc, *s);file[i] = *s;}file[i]='\0';}

static void Add_CrcOnly(char *p, unsigned short *crc, char *file)
{
	char *s;
	for(s=p;*s;s++)
		QCC_CRC_ProcessByte(crc, *s);
}
#define EAT_CRC(p) Add_CrcOnly(p, &crc, file)

unsigned short QCC_PR_WriteProgdefs (char *filename)
{
#define ADD_ONLY(p) QC_strlcat(file, p, sizeof(file))	//no crc (later changes)
	char file[PROGDEFS_MAX_SIZE];
	QCC_def_t	*d;
	int	f;
	unsigned short		crc;
//	int		c;

	file[0] = '\0';

	QCC_CRC_Init (&crc);

// print global vars until the first field is defined

	ADD_CRC("\n/* ");
	if (qcc_targetformat == QCF_HEXEN2 || qcc_targetformat == QCF_FTEH2)
		EAT_CRC("generated by hcc, do not modify");
	else
		EAT_CRC("file generated by qcc, do not modify");
	ADD_ONLY("File generated by FTEQCC, relevent for engine modding only, the generated crc must be the same as your engine expects.");
	ADD_CRC(" */\n\ntypedef struct");
	ADD_ONLY(" globalvars_s");
	ADD_CRC(qcva("\n{"));
	ADD_ONLY("\tint pad;\n"
		"\tint ofs_return[3];\n"	//makes it easier with the get globals func
		"\tint ofs_parm0[3];\n"
		"\tint ofs_parm1[3];\n"
		"\tint ofs_parm2[3];\n"
		"\tint ofs_parm3[3];\n"
		"\tint ofs_parm4[3];\n"
		"\tint ofs_parm5[3];\n"
		"\tint ofs_parm6[3];\n"
		"\tint ofs_parm7[3];\n");
	EAT_CRC(qcva("\tint\tpad[%i];\n", RESERVED_OFS));
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!strcmp (d->name, "end_sys_globals"))
			break;
		if (!*d->name)
			continue;
//		if (d->symbolheader->ofs<RESERVED_OFS)
//			continue;

		switch (d->type->type)
		{
		case ev_float:
			ADD_CRC(qcva("\tfloat\t%s;\n",d->name));
			break;
		case ev_vector:
			ADD_CRC(qcva("\tvec3_t\t%s;\n",d->name));
			if (d->deftail)
				d=d->deftail;	// skip the elements
			break;
		case ev_string:
			ADD_CRC(qcva("\tstring_t\t%s;\n",d->name));
			break;
		case ev_function:
			ADD_CRC(qcva("\tfunc_t\t%s;\n",d->name));
			break;
		case ev_entity:
			ADD_CRC(qcva("\tint\t%s;\n",d->name));
			break;
		case ev_integer:
			ADD_CRC(qcva("\tint\t%s;\n",d->name));
			break;
		default:
			ADD_CRC(qcva("\tint\t%s;\n",d->name));
			break;
		}
	}
	ADD_CRC("} globalvars_t;\n\n");

// print all fields
	ADD_CRC("typedef struct");
	ADD_ONLY(" entvars_s");
	ADD_CRC("\n{\n");
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!strcmp (d->name, "end_sys_fields"))
			break;

		if (d->type->type != ev_field)
			continue;

		switch (d->type->aux_type->type)
		{
		case ev_float:
			ADD_CRC(qcva("\tfloat\t%s;\n",d->name));
			break;
		case ev_vector:
			ADD_CRC(qcva("\tvec3_t\t%s;\n",d->name));
			if (d->deftail)
				d=d->deftail;	// skip the elements
			break;
		case ev_string:
			ADD_CRC(qcva("\tstring_t\t%s;\n",d->name));
			break;
		case ev_function:
			ADD_CRC(qcva("\tfunc_t\t%s;\n",d->name));
			break;
		case ev_entity:
			ADD_CRC(qcva("\tint\t%s;\n",d->name));
			break;
		case ev_integer:
			ADD_CRC(qcva("\tint\t%s;\n",d->name));
			break;
		default:
			ADD_CRC(qcva("\tint\t%s;\n",d->name));
			break;
		}
	}
	ADD_CRC("} entvars_t;\n\n");

/*
	///temp
	ADD_ONLY("//with this the crc isn't needed for fields.\n#ifdef FIELDSSTRUCT\nstruct fieldvars_s {\n\tint ofs;\n\tint type;\n\tchar *name;\n} fieldvars[] = {\n");
	f=0;
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!strcmp (d->name, "end_sys_fields"))
			break;

		if (d->type->type != ev_field)
			continue;
		if (f)
			ADD_ONLY(",\n");
		ADD_ONLY(qcva("\t{%i,\t%i,\t\"%s\"}",G_INT(d->ofs), d->type->aux_type->type, d->name));
		f = 1;
	}
	ADD_ONLY("\n};\n#endif\n\n");
	//end temp
*/

	ADD_ONLY(qcva("#define PROGHEADER_CRC %i\n", crc));

	if (QCC_CheckParm("-progdefs"))
	{
		printf ("writing %s\n", filename);
		f = SafeOpenWrite(filename, 16384);
		SafeWrite(f, file, strlen(file));
		SafeClose(f);
	}


	if (ForcedCRC)
		crc = ForcedCRC;

	switch (crc)
	{
	case 12923:	//#pragma sourcefile usage
		break;
	case 54730:
		if (verbose)
			printf("Recognised progs as QuakeWorld\n");
		break;
	case 5927:
		if (verbose)
			printf("Recognised progs as NetQuake server gamecode\n");
		break;

	case 26940:
		if (verbose)
			printf("Recognised progs as Quake pre-release...\n");
		break;

	case 38488:
		if (verbose)
			printf("Recognised progs as original Hexen2\n");
		break;
	case 26905:
		if (verbose)
			printf("Recognised progs as Hexen2 Mission Pack\n");
		break;
	case 14046:
		if (verbose)
			printf("Recognised progs as Hexen2 (demo)\n");
		break;

	case 22390: //EXT_CSQC_1
		if (verbose)
			printf("Recognised progs as an EXT_CSQC_1 module\n");
		break;
	case 17105:
	case 32199:	//outdated ext_csqc
		QCC_PR_Warning(WARN_SYSTEMCRC, NULL, 0, "Recognised progs as outdated CSQC module\n");
		break;
	case 52195:	//this is what DP requires. don't print it as the warning that it is as that would royally piss off xonotic and their use of -Werror.
		printf("Recognised progs as DP-specific CSQC module\n");
		break;
	case 10020:
		if (verbose)
			printf("Recognised progs as a DP/FTE Menu module\n");
		break;

	case 32401:
		QCC_PR_Warning(WARN_SYSTEMCRC, NULL, 0, "please update your tenebrae system defs.\n");
		break;
	default:
		QCC_PR_Warning(WARN_SYSTEMCRC, NULL, 0, "progs CRC not recognised from quake nor clones\n");
		break;
	}


	return crc;
}


/*void QCC_PrintFunction (char *name)
{
	int		i;
	QCC_dstatement_t	*ds;
	QCC_dfunction_t		*df;

	for (i=0 ; i<numfunctions ; i++)
		if (!strcmp (name, strings + functions[i].s_name))
			break;
	if (i==numfunctions)
		QCC_Error (ERR_NOFUNC, "No function named \"%s\"", name);
	df = functions + i;

	printf ("Statements for function %s:\n", name);
	ds = statements + df->first_statement;
	while (1)
	{
		QCC_PR_PrintStatement (ds);
		if (!ds->op)
			break;
		ds++;
	}
}*/
/*
void QCC_PrintOfs(unsigned int ofs)
{
	int i;
	bool printfunc;
	QCC_dstatement_t	*ds;
	QCC_dfunction_t		*df;

	for (i=0 ; i<numfunctions ; i++)
	{
		df = functions + i;
		ds = statements + df->first_statement;
		printfunc = false;
		while (1)
		{
			if (!ds->op)
				break;
			if (ds->a == ofs || ds->b == ofs || ds->c == ofs)
			{
				QCC_PR_PrintStatement (ds);
				printfunc = true;
			}
			ds++;
		}
		if (printfunc)
		{
			QCC_PrintFunction(strings + functions[i].s_name);
			printf(" \n \n");
		}
	}

}
*/
/*
==============================================================================

DIRECTORY COPYING / PACKFILE CREATION

==============================================================================
*/

typedef struct
{
	char	name[56];
	int		filepos, filelen;
} packfile_t;

typedef struct
{
	char	id[4];
	int		dirofs;
	int		dirlen;
} packheader_t;

packfile_t	pfiles[4096], *pf;
int			packhandle;
int			packbytes;


/*
============
CreatePath
============
*/
void	QCC_CreatePath (char *path)
{
	/*
	char	*ofs;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
#ifdef QCC
			mkdir(path);
#else
			QCC_mkdir (path);
#endif
			*ofs = '/';
		}
	}
	*/
}


/*
===========
PackFile

Copy a file into the pak file
===========
*/
void QCC_PackFile (char *src, char *name)
{
	int		remaining;
#if 1
	char	*f;
#else
	int		in;
	int		count;
	char	buf[4096];
#endif


	if ( (qbyte *)pf - (qbyte *)pfiles > sizeof(pfiles) )
		QCC_Error (ERR_TOOMANYPAKFILES, "Too many files in pak file");

#if 1
	f = FS_ReadToMem(src, NULL, &remaining);
	if (!f)
	{
		printf ("%64s : %7s\n", name, "");
//		QCC_Error("Failed to open file %s", src);
		return;
	}

	pf->filepos = PRLittleLong (SafeSeek (packhandle, 0, SEEK_CUR));
	pf->filelen = PRLittleLong (remaining);
	strcpy (pf->name, name);
	printf ("%64s : %7i\n", pf->name, remaining);

	packbytes += remaining;

	SafeWrite (packhandle, f, remaining);

	FS_CloseFromMem(f);
#else
	in = SafeOpenRead (src);
	remaining = filelength (in);

	pf->filepos = PRLittleLong (lseek (packhandle, 0, SEEK_CUR));
	pf->filelen = PRLittleLong (remaining);
	strcpy (pf->name, name);
	printf ("%64s : %7i\n", pf->name, remaining);

	packbytes += remaining;

	while (remaining)
	{
		if (remaining < sizeof(buf))
			count = remaining;
		else
			count = sizeof(buf);
		SafeRead (in, buf, count);
		SafeWrite (packhandle, buf, count);
		remaining -= count;
	}

	close (in);
#endif
	pf++;
}


/*
===========
CopyFile

Copies a file, creating any directories needed
===========
*/
void QCC_CopyFile (char *src, char *dest)
{
	/*
	int		in, out;
	int		remaining, count;
	char	buf[4096];

	print ("%s to %s\n", src, dest);

	in = SafeOpenRead (src);
	remaining = filelength (in);

	QCC_CreatePath (dest);
	out = SafeOpenWrite (dest, remaining+10);

	while (remaining)
	{
		if (remaining < sizeof(buf))
			count = remaining;
		else
			count = sizeof(buf);
		SafeRead (in, buf, count);
		SafeWrite (out, buf, count);
		remaining -= count;
	}

	close (in);
	SafeClose (out);
	*/
}


/*
===========
CopyFiles
===========
*/

void _QCC_CopyFiles (int blocknum, int copytype, char *srcdir, char *destdir)
{
	int i;
	int		dirlen;
	unsigned short		crc;
	packheader_t	header;
	char	name[1024];
	char	srcfile[1024], destfile[1024];

	packbytes = 0;

	if (copytype == 2)
	{
		pf = pfiles;
		packhandle = SafeOpenWrite (destdir, 1024*1024);
		SafeWrite (packhandle, &header, sizeof(header));
	}

	for (i=0 ; i<numsounds ; i++)
	{
		if (precache_sound[i].block != blocknum)
			continue;
		sprintf (srcfile,"%s%s",srcdir, precache_sound[i].name);
		sprintf (destfile,"%s%s",destdir, precache_sound[i].name);
		if (copytype == 1)
			QCC_CopyFile (srcfile, destfile);
		else
			QCC_PackFile (srcfile, precache_sound[i].name);
	}
	for (i=0 ; i<nummodels ; i++)
	{
		if (precache_model[i].block != blocknum)
			continue;
		sprintf (srcfile,"%s%s",srcdir, precache_model[i].name);
		sprintf (destfile,"%s%s",destdir, precache_model[i].name);
		if (copytype == 1)
			QCC_CopyFile (srcfile, destfile);
		else
			QCC_PackFile (srcfile, precache_model[i].name);
	}
	for (i=0 ; i<numtextures ; i++)
	{
		if (precache_texture[i].block != blocknum)
			continue;

		{
			sprintf (name, "%s", precache_texture[i].name);
			sprintf (srcfile,"%s%s",srcdir, name);
			sprintf (destfile,"%s%s",destdir, name);
			if (copytype == 1)
				QCC_CopyFile (srcfile, destfile);
			else
				QCC_PackFile (srcfile, name);
		}
		{
			sprintf (name, "%s.bmp", precache_texture[i].name);
			sprintf (srcfile,"%s%s",srcdir, name);
			sprintf (destfile,"%s%s",destdir, name);
			if (copytype == 1)
				QCC_CopyFile (srcfile, destfile);
			else
				QCC_PackFile (srcfile, name);
		}
		{
			sprintf (name, "%s.tga", precache_texture[i].name);
			sprintf (srcfile,"%s%s",srcdir, name);
			sprintf (destfile,"%s%s",destdir, name);
			if (copytype == 1)
				QCC_CopyFile (srcfile, destfile);
			else
				QCC_PackFile (srcfile, name);
		}
	}
	for (i=0 ; i<numfiles ; i++)
	{
		if (precache_file[i].block != blocknum)
			continue;
		sprintf (srcfile,"%s%s",srcdir, precache_file[i].name);
		sprintf (destfile,"%s%s",destdir, precache_file[i].name);
		if (copytype == 1)
			QCC_CopyFile (srcfile, destfile);
		else
			QCC_PackFile (srcfile, precache_file[i].name);
	}

	if (copytype == 2)
	{
		header.id[0] = 'P';
		header.id[1] = 'A';
		header.id[2] = 'C';
		header.id[3] = 'K';
		dirlen = (qbyte *)pf - (qbyte *)pfiles;
		header.dirofs = PRLittleLong(SafeSeek (packhandle, 0, SEEK_CUR));
		header.dirlen = PRLittleLong(dirlen);

		SafeWrite (packhandle, pfiles, dirlen);

		SafeSeek (packhandle, 0, SEEK_SET);
		SafeWrite (packhandle, &header, sizeof(header));
		SafeClose (packhandle);

	// do a crc of the file
		QCC_CRC_Init (&crc);
		for (i=0 ; i<dirlen ; i++)
			QCC_CRC_ProcessByte (&crc, ((qbyte *)pfiles)[i]);

		i = pf - pfiles;
		printf ("%i files packed in %i bytes (%i crc)\n",i, packbytes, crc);
	}
}

void QCC_CopyFiles (void)
{
	char *s;
	char	srcdir[1024], destdir[1024];
	int		p;

	if (verbose)
	{
		if (numsounds > 0)
			printf ("%3i unique precache_sounds\n", numsounds);
		if (nummodels > 0)
			printf ("%3i unique precache_models\n", nummodels);
		if (numtextures > 0)
			printf ("%3i unique precache_textures\n", numtextures);
		if (numfiles > 0)
			printf ("%3i unique precache_files\n", numfiles);
	}

	p = QCC_CheckParm ("-copy");
	if (p && p < myargc-2)
	{	// create a new directory tree

		strcpy (srcdir, myargv[p+1]);
		strcpy (destdir, myargv[p+2]);
		if (srcdir[strlen(srcdir)-1] != '/')
			strcat (srcdir, "/");
		if (destdir[strlen(destdir)-1] != '/')
			strcat (destdir, "/");

		_QCC_CopyFiles(0, 1, srcdir, destdir);
		return;
	}

	for ( p = 0; p < 5; p++)
	{
		s = QCC_Packname[p];
		if (!*s)
			continue;
		strcpy(destdir, s);
		strcpy(srcdir, "");
		_QCC_CopyFiles(p+1, 2, srcdir, destdir);
	}
	return;
	/*

	blocknum = 1;
	p = QCC_CheckParm ("-pak2");
	if (p && p <myargc-2)
		blocknum = 2;
	else
		p = QCC_CheckParm ("-pak");
	if (p && p < myargc-2)
	{	// create a pak file
		strcpy (srcdir, myargv[p+1]);
		strcpy (destdir, myargv[p+2]);
		if (srcdir[strlen(srcdir)-1] != '/')
			strcat (srcdir, "/");
		DefaultExtension (destdir, ".pak");


		copytype = 2;

		_QCC_CopyFiles(blocknum, copytype, srcdir, destdir);
	}
	*/
}

//============================================================================

#ifdef _WIN32
#define WINDOWSARG(x) x
#else
#define WINDOWSARG(x) false
#endif

void QCC_PR_CommandLinePrecompilerOptions (void)
{
	CompilerConstant_t *cnst;
	int             i, j, p;
	char *name, *val;
	pbool werror = false;
	qcc_nopragmaoptimise = false;

	for (i = 1;i<myargc;i++)
	{
		if ( !strcmp(myargv[i], "-v") )
			verbose++;	//verbose
		else if ( !strcmp(myargv[i], "-srcfile") )
		{
			if (++i == myargc)
				break;
			for (j = 0; j < numsourcefiles; j++)
			{
				if (!strcmp(sourcefileslist[j], myargv[i]))
					break;
			}
			if (j == numsourcefiles)
			{
				if (numsourcefiles < MAXSOURCEFILESLIST)
					strcpy(sourcefileslist[numsourcefiles++], myargv[i]);
				else
					QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "too many -srcfile arguments");
			}
		}
		else if ( !strcmp(myargv[i], "-src") )
		{
			i++;
			strcpy (qccmsourcedir, myargv[i]);
			strcat (qccmsourcedir, "/");
		}
		else if ( !strcmp(myargv[i], "-o") )
			;	//explicit output file
		else if ( !strcmp(myargv[i], "-qc") )
			QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Argument %s is experimental", myargv[i]);	//compile without linking. output cannot be read by engines.
		else if ( !strcmp(myargv[i], "-progdefs") )
			;	//write progdefs.h
		else if ( !strcmp(myargv[i], "-copy") )
			;	//copy files / write pak files
		else if ( !strcmp(myargv[i], "-bspmodels") )
			QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Argument %s is not supported", myargv[i]);
		else if ( !strcmp(myargv[i], "-h2") || !strcmp(myargv[i], "-fteh2")  || !strcmp(myargv[i], "-fte") || !strcmp(myargv[i], "-dp")  )
			;	//various targets
		else if ( !strcmp(myargv[i], "-pak") || !strcmp(myargv[i], "-pak2") )
			QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Argument %s is not supported", myargv[i]);
		else

		//compiler constant
		if ( !strncmp(myargv[i], "-D", 2) )
		{
			name = myargv[i] + 2;
			val = strchr(name, '=');
			if (val)
			{
				*val = '\0';
				val++;
			}
			cnst = QCC_PR_DefineName(name);
			if (val)
			{
				cnst->value = qccHunkAlloc(strlen(val)+1);
				memcpy(cnst->value, val, strlen(val)+1);
			}
		}
		else if ( !strncmp(myargv[i], "-I", 2) )
		{
			name = myargv[i] + 2;
			if (!*name && i+1<myargc)
				name = myargv[++i];

			QCC_PR_AddIncludePath(name);
		}

		//optimisations.
		else if ( !strnicmp(myargv[i], "-O", 2) || WINDOWSARG(!strnicmp(myargv[i], "/O", 2)) )
		{
			qcc_nopragmaoptimise = true;
			p = 0;
			if (myargv[i][2] >= '0' && myargv[i][2] <= '3')
			{
			}
			else if (!strnicmp(myargv[i]+2, "no-", 3))
			{
				if (myargv[i][5])
				{
					for (p = 0; optimisations[p].enabled; p++)
					{
						if ((*optimisations[p].abbrev && !stricmp(myargv[i]+5, optimisations[p].abbrev)) || !stricmp(myargv[i]+5, optimisations[p].fullname))
						{
							*optimisations[p].enabled = false;
							break;
						}
					}
				}
			}
			else
			{
				if (myargv[i][2])
					for (p = 0; optimisations[p].enabled; p++)
						if ((*optimisations[p].abbrev && !stricmp(myargv[i]+2, optimisations[p].abbrev)) || !stricmp(myargv[i]+2, optimisations[p].fullname))
						{
							*optimisations[p].enabled = true;
							break;
						}
			}
			if (!optimisations[p].enabled)
				QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Unrecognised optimisation parameter (%s)", myargv[i]);
		}

		else if ( !strnicmp(myargv[i], "-K", 2) || WINDOWSARG(!strnicmp(myargv[i], "/K", 2)) )
		{
			p = 0;
			if (!strnicmp(myargv[i]+2, "no-", 3))
			{
				for (p = 0; compiler_flag[p].enabled; p++)
					if (!stricmp(myargv[i]+5, compiler_flag[p].abbrev))
					{
						*compiler_flag[p].enabled = false;
						break;
					}
			}
			else
			{
				for (p = 0; compiler_flag[p].enabled; p++)
					if (!stricmp(myargv[i]+2, compiler_flag[p].abbrev))
					{
						*compiler_flag[p].enabled = true;
						break;
					}
			}

			if (!compiler_flag[p].enabled)
				QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Unrecognised keyword parameter (%s)", myargv[i]);
		}
		else if ( !strnicmp(myargv[i], "-F", 2) || WINDOWSARG(!strnicmp(myargv[i], "/F", 2)) )
		{
			pbool state;
			const char *arg;
			p = 0;
			if (!strnicmp(myargv[i]+2, "no-", 3))
			{
				arg = myargv[i]+5;
				state = false;
			}
			else
			{
				arg = myargv[i]+2;
				state = true;
			}

			for (p = 0; compiler_flag[p].enabled; p++)
				if (!stricmp(arg, compiler_flag[p].abbrev))
				{
					*compiler_flag[p].enabled = state;
					break;
				}

			if (!compiler_flag[p].enabled)
			{
				//compat flags.
				if (!stricmp(arg, "short-logic"))
					opt_logicops = state;
				else if (!stricmp(arg, "correct-logic"))
					flag_ifvector = flag_vectorlogic = state;
				else if (!stricmp(arg, "false-empty-strings"))
					flag_ifstring = state;
				else if (!stricmp(arg, "true-empty-strings"))
					flag_brokenifstring = state;
				else
					QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Unrecognised flag parameter (%s)", myargv[i]);
			}
		}


		else if ( !strncmp(myargv[i], "-T", 2) || WINDOWSARG(!strncmp(myargv[i], "/T", 2)) )
		{
			p = 0;
			if (!strcmp("parse", myargv[i]+2))
				parseonly = true;
			else
			{
				for (p = 0; targets[p].name; p++)
					if (!stricmp(myargv[i]+2, targets[p].name))
					{
						qcc_targetformat = targets[p].target;
						break;
					}

				if (!targets[p].name)
					QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Unrecognised target parameter (%s)", myargv[i]);
			}
		}

		else if ( !strnicmp(myargv[i], "-W", 2) || WINDOWSARG(!strnicmp(myargv[i], "/W", 2)) )
		{
			if (!stricmp(myargv[i]+2, "all"))
			{
				for (j = 0; j < ERR_PARSEERRORS; j++)
					if (qccwarningaction[j] == WA_IGNORE)
					{
						if (j != WARN_FTE_SPECIFIC &&		//kinda annoying when its actually valid code.
							j != WARN_NOTREFERENCEDCONST &&	//warning about every single constant is annoying as heck. note that this includes both stuff like MOVETYPE_ and builtins.
							j != WARN_EXTRAPRECACHE)		//we can't guarentee that we can parse this correctly. this warning is thus a common false positive. its available with -Wextra, and there's intrinsics to reduce false positives.
							qccwarningaction[j] = WA_WARN;
					}
			}
			else if (!stricmp(myargv[i]+2, "extra"))
			{
				for (j = 0; j < ERR_PARSEERRORS; j++)
					if (qccwarningaction[j] == WA_IGNORE)
						qccwarningaction[j] = WA_WARN;
			}
			else if (!stricmp(myargv[i]+2, "none"))
			{
				for (j = 0; j < ERR_PARSEERRORS; j++)
					qccwarningaction[j] = WA_IGNORE;
			}
			else if(!stricmp(myargv[i]+2, "error"))
			{
				werror = true;
			}
			else if (!stricmp(myargv[i]+2, "no-mundane"))
			{	//disable mundane performance/efficiency/blah warnings that don't affect code.
				qccwarningaction[WARN_SAMENAMEASGLOBAL] = WA_IGNORE;
				qccwarningaction[WARN_DUPLICATEDEFINITION] = WA_IGNORE;
				qccwarningaction[WARN_CONSTANTCOMPARISON] = WA_IGNORE;
				qccwarningaction[WARN_ASSIGNMENTINCONDITIONAL] = WA_IGNORE;
				qccwarningaction[WARN_DEADCODE] = WA_IGNORE;
				qccwarningaction[WARN_NOTREFERENCEDCONST] = WA_IGNORE;
				qccwarningaction[WARN_NOTREFERENCED] = WA_IGNORE;
				qccwarningaction[WARN_POINTLESSSTATEMENT] = WA_IGNORE;
				qccwarningaction[WARN_ASSIGNMENTTOCONSTANTFUNC] = WA_IGNORE;
				qccwarningaction[WARN_BADPRAGMA] = WA_IGNORE;	//C specs say that these should be ignored. We're close enough to C that I consider that a valid statement.
				qccwarningaction[WARN_IDENTICALPRECOMPILER] = WA_IGNORE;
				qccwarningaction[WARN_UNDEFNOTDEFINED] = WA_IGNORE;
				qccwarningaction[WARN_EXTRAPRECACHE] = WA_IGNORE;
				qccwarningaction[WARN_CORRECTEDRETURNTYPE] = WA_IGNORE;
				qccwarningaction[WARN_NOTUTF8] = WA_IGNORE;
				qccwarningaction[WARN_SELFNOTTHIS] = WA_IGNORE;
			}
			else
			{
				p = -1;
				if (!strnicmp(myargv[i]+2, "error-", 6))
				{
					p = QCC_WarningForName(myargv[i]+8);
					if (p >= 0)
						qccwarningaction[p] = WA_ERROR;
				}
				else if (!strnicmp(myargv[i]+2, "no-", 3))
				{
					p = QCC_WarningForName(myargv[i]+5);
					if (p >= 0)
						qccwarningaction[p] = WA_IGNORE;
				}
				else
				{
					p = QCC_WarningForName(myargv[i]+2);
					if (p >= 0)
						qccwarningaction[p] = WA_WARN;
				}

				if (p < 0)
					QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Unrecognised warning parameter (%s)", myargv[i]);
			}
		}
		else if (*myargv[i] == '-' || WINDOWSARG(*myargv[i] == '/'))
			QCC_PR_Warning(0, NULL, WARN_BADPARAMS, "Unrecognised parameter (%s)", myargv[i]);
	}

	if (werror)
	{
		for (j = 0; j < ERR_PARSEERRORS; j++)
			if (qccwarningaction[j])
				qccwarningaction[j] = WA_ERROR;
	}
}

/*
============
main
============
*/

char	*qccmsrc;
char	*qccmsrc2;
char	qccmfilename[1024];
char	qccmprogsdat[1024];

void QCC_FinishCompile(void);


void SetEndian(void);



void QCC_SetDefaultProperties (void)
{
	int level;
	int i;
#ifdef _WIN32
#define FWDSLASHARGS 1
#else
#define FWDSLASHARGS 0
#endif

	Hash_InitTable(&compconstantstable, MAX_CONSTANTS, qccHunkAlloc(Hash_BytesForBuckets(MAX_CONSTANTS)));

	ForcedCRC = 0;
	defaultstatic = 0;
	verbose = 0;
	*qccmsourcedir = 0;
	QCC_PR_CloseProcessor();

	QCC_PR_DefineName("FTEQCC");

	if ((FWDSLASHARGS && QCC_CheckParm("/O0")) || QCC_CheckParm("-O0"))
		level = 0;
	else if ((FWDSLASHARGS && QCC_CheckParm("/O1")) || QCC_CheckParm("-O1"))
		level = 1;
	else if ((FWDSLASHARGS && QCC_CheckParm("/O2")) || QCC_CheckParm("-O2"))
		level = 2;
	else if ((FWDSLASHARGS && QCC_CheckParm("/O3")) || QCC_CheckParm("-O3"))
		level = 3;
	else
		level = -1;

	if (level == -1)
	{
		for (i = 0; optimisations[i].enabled; i++)
		{
			if (optimisations[i].flags & FLAG_ASDEFAULT)
				*optimisations[i].enabled = true;
			else
				*optimisations[i].enabled = false;
		}
	}
	else
	{
		for (i = 0; optimisations[i].enabled; i++)
		{
			if (level >= optimisations[i].optimisationlevel)
				*optimisations[i].enabled = true;
			else
				*optimisations[i].enabled = false;
		}
	}

	if (QCC_CheckParm ("-h2"))
		qcc_targetformat = QCF_HEXEN2;
	else if (QCC_CheckParm ("-fte"))
		qcc_targetformat = QCF_FTE;
	else if (QCC_CheckParm ("-fteh2"))
		qcc_targetformat = QCF_FTEH2;
	else if (QCC_CheckParm ("-dp"))
		qcc_targetformat = QCF_DARKPLACES;
	else
		qcc_targetformat = QCF_STANDARD;


	//enable all warnings
	for (i = 0; i < ERR_PARSEERRORS; i++)
		qccwarningaction[i] = WA_WARN;
	for (; i < WARN_MAX; i++)
		qccwarningaction[i] = WA_ERROR;

	//play with default warnings.
	qccwarningaction[WARN_NOTREFERENCEDCONST] = WA_IGNORE;
	qccwarningaction[WARN_MACROINSTRING] = WA_IGNORE;
//	qccwarningaction[WARN_ASSIGNMENTTOCONSTANT] = WA_IGNORE;
	qccwarningaction[WARN_EXTRAPRECACHE] = WA_IGNORE;
	qccwarningaction[WARN_DEADCODE] = WA_IGNORE;
	qccwarningaction[WARN_FTE_SPECIFIC] = WA_IGNORE;
	qccwarningaction[WARN_EXTENSION_USED] = WA_IGNORE;
	qccwarningaction[WARN_IFSTRING_USED] = WA_IGNORE;
	qccwarningaction[WARN_CORRECTEDRETURNTYPE] = WA_IGNORE;
	qccwarningaction[WARN_NOTUTF8] = WA_IGNORE;
	qccwarningaction[WARN_UNINITIALIZED] = WA_IGNORE;	//not sure about this being ignored by default.
	qccwarningaction[WARN_SELFNOTTHIS] = WA_IGNORE;
	qccwarningaction[WARN_EVILPREPROCESSOR] = WA_WARN;//FIXME: make into WA_ERROR;
	qccwarningaction[WARN_IDENTICALPRECOMPILER] = WA_IGNORE;

	if (qcc_targetformat == QCF_HEXEN2 || qcc_targetformat == QCF_FTEH2)
		qccwarningaction[WARN_CASEINSENSITIVEFRAMEMACRO] = WA_IGNORE;	//hexenc consides these fair game.

	if (QCC_CheckParm ("-Fqccx"))
	{
		qccwarningaction[WARN_DENORMAL] = WA_IGNORE;	//this is just too spammy
		qccwarningaction[WARN_LAXCAST] = WA_IGNORE;	//more plausable, but still too spammy. easier to fix at least.
	}

	//Check the command line
	QCC_PR_CommandLinePrecompilerOptions();


	if (qcc_targetformat == QCF_HEXEN2 || qcc_targetformat == QCF_FTEH2)	//force on the thinktime keyword if hexen2 progs.
	{
		keyword_thinktime = true;
		keyword_until = true;
		keyword_loop = true;
	}

	if ((FWDSLASHARGS && QCC_CheckParm("/Debug")))	//disable any debug optimisations
	{
		for (i = 0; optimisations[i].enabled; i++)
		{
			if (optimisations[i].flags & FLAG_KILLSDEBUGGERS)
				*optimisations[i].enabled = false;
		}
	}
}

//builds a list of files, pretends that they came from a progs.src
//FIXME: use sourcedir!
int QCC_FindQCFiles(const char *sourcedir)
{
#ifdef _WIN32
	WIN32_FIND_DATA fd;
	HANDLE h;
#endif

	int numfiles = 0, i, j;
	char *filelist[256], *temp;


	qccmsrc = qccHunkAlloc(8192);
	strcat(qccmsrc, "progs.dat\n");//"#pragma PROGS_DAT progs.dat\n");

#if defined(_WIN32) && !defined(WINRT)
	h = FindFirstFile("*.qc", &fd);
	if (h == INVALID_HANDLE_VALUE)
		return 0;

	do
	{
		filelist[numfiles] = qccHunkAlloc (strlen(fd.cFileName)+1);
		strcpy(filelist[numfiles], fd.cFileName);
		numfiles++;
	} while(FindNextFile(h, &fd)!=0);
	FindClose(h);
#else
	printf("-Facc is not supported on this platform. Please make a progs.src file instead\n");
#endif

	//Sort alphabetically.
	//bubble. :(

	for (i = 0; i < numfiles-1; i++)
	{
		for (j = i+1; j < numfiles; j++)
		{
			if (stricmp(filelist[i], filelist[j]) > 0)
			{
				temp = filelist[j];
				filelist[j] = filelist[i];
				filelist[i] = temp;
			}
		}
	}
	for (i = 0; i < numfiles; i++)
	{
		strcat(qccmsrc, filelist[i]);
		strcat(qccmsrc, "\n");
//		strcat(qccmsrc, "#include \"");
//		strcat(qccmsrc, filelist[i]);
//		strcat(qccmsrc, "\"\n");
	}

	return numfiles;
}


void QCC_GenerateRelativePath(char *dest, size_t destsize, char *base, char *relative)
{
	int p;
	char *s1, *s2;
	QC_strlcpy (dest, base, destsize);
	s1 = strchr(dest, '\\');
	s2 = strchr(dest, '/');
	if (s2 > s1)
		s1 = s2;
	if (s1)
		*s1 = 0;
	else
		*dest = 0;

	p=0;
	s2 = relative;
	for (;;)
	{
		if (!strncmp(s2, "./", 2))
			s2+=2;
		else if(!strncmp(s2, "../", 3))
		{
			s2+=3;
			p++;
		}
		else
			break;
	}
	for (s1=dest+strlen(dest)-1;p && s1>=dest; s1--)
	{
		if (*s1 == '/' || *s1 == '\\')
		{
			*s1 = '\0';
			p--;
		}
	}
	if (*dest)
	{
		if (p)
		{	//we were still looking for a separator, but didn't find one, so kill the entire path.
			QC_strlcpy(dest, "", destsize);
			p--;
		}
		else
			QC_strlcat(dest, "/", destsize);
	}
	QC_strlcat(dest, s2, destsize);

	while (p>0 && strlen(dest)+3 < destsize)
	{
		memmove(dest+3, dest, strlen(dest)+1);
		dest[0] = '.';
		dest[1] = '.';
		dest[2] = '/';
		p--;
	}
}

int qcc_compileactive = false;
extern int accglobalsblock;
char *originalqccmsrc;	//for autoprototype.
pbool QCC_main (int argc, char **argv)	//as part of the quake engine
{
	extern int			pr_bracelevel;
	time_t long_time;
	extern QCC_type_t *pr_classtype;

	int		p;
	extern int qccpersisthunk;

	char *s;

	s_filen = "cmdline";
	s_filed = 0;
	pr_source_line = 0;

	if (numsourcefiles && currentsourcefile == numsourcefiles)
	{
		numsourcefiles = 0;
		return false;
	}
	else if (!numsourcefiles)
		currentsourcefile = 0;

	if (currentsourcefile && qccpersisthunk && numsourcefiles)
		QCC_PR_ResetErrorScope();	//don't clear the ram if we're retaining def info
	else
	{
		memset(sourcefilesdefs, 0, sizeof(sourcefilesdefs));
		sourcefilesnumdefs = 0;
		if (!PreCompile())
			return false;
	}

	SetEndian();

	myargc = argc;
	myargv = argv;
	pr_scope = NULL;
	pr_classtype = NULL;
	locals_marshalled = 0;

	qcc_compileactive = true;

	pHash_Get = &Hash_Get;
	pHash_GetNext = &Hash_GetNext;
	pHash_Add = &Hash_Add;
	pHash_RemoveData = &Hash_RemoveData;

	MAX_REGS		= 1<<17;
	MAX_STRINGS		= 1000000;
	MAX_GLOBALS		= 1<<17;
	MAX_FIELDS		= 1<<12;
	MAX_STATEMENTS	= 0x80000;
	MAX_FUNCTIONS	= 16384;
	maxtypeinfos	= 16384;
	MAX_CONSTANTS	= 4096;

	compressoutput = 0;

	p = externs->FileSize("qcc.cfg");
//	if (p < 0)
//		p = externs->FileSize("src/qcc.cfg");
	if (p>0)
	{
		s = qccHunkAlloc(p+1);
		s = (char*)externs->ReadFile("qcc.cfg", s, p, NULL);

		while(1)
		{
			s = QCC_COM_Parse(s);
			if (!strcmp(qcc_token, "MAX_REGS"))
			{
				s = QCC_COM_Parse(s);
				MAX_REGS = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "MAX_STRINGS")) {
				s = QCC_COM_Parse(s);
				MAX_STRINGS = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "MAX_GLOBALS")) {
				s = QCC_COM_Parse(s);
				MAX_GLOBALS = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "MAX_FIELDS")) {
				s = QCC_COM_Parse(s);
				MAX_FIELDS = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "MAX_STATEMENTS")) {
				s = QCC_COM_Parse(s);
				MAX_STATEMENTS = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "MAX_FUNCTIONS")) {
				s = QCC_COM_Parse(s);
				MAX_FUNCTIONS = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "MAX_TYPES")) {
				s = QCC_COM_Parse(s);
				maxtypeinfos = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "MAX_TEMPS")) {
				s = QCC_COM_Parse(s);
				max_temps = atoi(qcc_token);
			} else if (!strcmp(qcc_token, "CONSTANTS")) {
				s = QCC_COM_Parse(s);
				MAX_CONSTANTS = atoi(qcc_token);
			}
			else if (!s)
				break;
			else
				printf("Bad token in qcc.cfg file\n");
		}
	}
	/* don't try to be clever
	else if (p < 0)
	{
		s = qccHunkAlloc(8192);
		sprintf(s, "MAX_REGS\t%i\r\nMAX_STRINGS\t%i\r\nMAX_GLOBALS\t%i\r\nMAX_FIELDS\t%i\r\nMAX_STATEMENTS\t%i\r\nMAX_FUNCTIONS\t%i\r\nMAX_TYPES\t%i\r\n",
					MAX_REGS,    MAX_STRINGS,    MAX_GLOBALS,    MAX_FIELDS,    MAX_STATEMENTS,    MAX_FUNCTIONS,    maxtypeinfos);
		externs->WriteFile("qcc.cfg", s, strlen(s));
	}
	*/

	time(&long_time);
	strftime(QCC_copyright, sizeof(QCC_copyright),  "Compiled [%Y/%m/%d]. ", localtime( &long_time ));
	QC_strlcat(QCC_copyright, QCC_VersionString(), sizeof(QCC_copyright));
	for (p = 0; p < 5; p++)
		strcpy(QCC_Packname[p], "");

	for (p = 0; compiler_flag[p].enabled; p++)
	{
		*compiler_flag[p].enabled = compiler_flag[p].flags & FLAG_ASDEFAULT;
	}

	parseonly = autoprototyped = autoprototype = false;
	QCC_SetDefaultProperties();
	autoprototype |= parseonly;

	optres_shortenifnots = 0;
	optres_overlaptemps = 0;
	optres_noduplicatestrings = 0;
	optres_constantarithmatic = 0;
	optres_nonvec_parms = 0;
	optres_constant_names = 0;
	optres_constant_names_strings = 0;
	optres_precache_file = 0;
	optres_filenames = 0;
	optres_assignments = 0;
	optres_unreferenced = 0;
	optres_function_names = 0;
	optres_locals = 0;
	optres_dupconstdefs = 0;
	optres_return_only = 0;
	optres_compound_jumps = 0;
//	optres_comexprremoval = 0;
	optres_stripfunctions = 0;
	optres_locals_overlapping = 0;
	optres_logicops = 0;
	optres_inlines = 0;

	optres_test1 = 0;
	optres_test2 = 0;

	accglobalsblock = 0;


	tempsused = 0;

	QCC_PurgeTemps();

	strings = (void *)qccHunkAlloc(sizeof(char) * MAX_STRINGS);
	strofs = 2;

	statements = (void *)qccHunkAlloc(sizeof(QCC_statement_t) * MAX_STATEMENTS);
	numstatements = 0;

	functions = (void *)qccHunkAlloc(sizeof(QCC_function_t) * MAX_FUNCTIONS);
	numfunctions=0;

	pr_bracelevel = 0;

	qcc_pr_globals = (void *)qccHunkAlloc(sizeof(float) * (MAX_REGS + MAX_LOCALS + MAX_TEMPS));
	numpr_globals=0;

	Hash_InitTable(&globalstable, MAX_REGS/2, qccHunkAlloc(Hash_BytesForBuckets(MAX_REGS/2)));
	Hash_InitTable(&localstable, 128, qccHunkAlloc(Hash_BytesForBuckets(128)));
	Hash_InitTable(&floatconstdefstable, MAX_REGS/2+1, qccHunkAlloc(Hash_BytesForBuckets(MAX_REGS/2+1)));
	Hash_InitTable(&stringconstdefstable, MAX_REGS/2, qccHunkAlloc(Hash_BytesForBuckets(MAX_REGS/2)));
	Hash_InitTable(&stringconstdefstable_trans, 1000, qccHunkAlloc(Hash_BytesForBuckets(1000)));
	dotranslate_count = 0;

//	pr_global_defs = (QCC_def_t **)qccHunkAlloc(sizeof(QCC_def_t *) * MAX_REGS);

	qcc_globals = (void *)qccHunkAlloc(sizeof(QCC_ddef_t) * MAX_GLOBALS);
	numglobaldefs=0;

	fields = (void *)qccHunkAlloc(sizeof(QCC_ddef_t) * MAX_FIELDS);
	numfielddefs=0;

memset(pr_immediate_string, 0, sizeof(pr_immediate_string));

	precache_sound = (void *)qccHunkAlloc(sizeof(*precache_sound)*QCC_MAX_SOUNDS);
	numsounds=0;
	precache_texture = (void *)qccHunkAlloc(sizeof(*precache_texture)*QCC_MAX_TEXTURES);
	numtextures=0;
	precache_model = (void *)qccHunkAlloc(sizeof(*precache_model)*QCC_MAX_MODELS);
	nummodels=0;
	precache_file = (void *)qccHunkAlloc(sizeof(*precache_file)*QCC_MAX_FILES);
	numfiles = 0;

	qcc_typeinfo = (void *)qccHunkAlloc(sizeof(QCC_type_t)*maxtypeinfos);
	numtypeinfos = 0;

	qcc_tempofs = qccHunkAlloc(sizeof(int) * max_temps);
	tempsstart = 0;

	bodylessfuncs=0;

	memset(&pr, 0, sizeof(pr));
#ifdef MAX_EXTRA_PARMS
	memset(&extra_parms, 0, sizeof(extra_parms));
#endif

	if ( QCC_CheckParm ("/?") || QCC_CheckParm ("?") || QCC_CheckParm ("-?") || QCC_CheckParm ("-help") || QCC_CheckParm ("--help"))
	{
		printf ("qcc looks for progs.src in the current directory.\n");
		printf ("to look in a different directory: qcc -src <directory>\n");
//		printf ("to build a clean data tree: qcc -copy <srcdir> <destdir>\n");
//		printf ("to build a clean pak file: qcc -pak <srcdir> <packfile>\n");
//		printf ("to bsp all bmodels: qcc -bspmodels <gamedir>\n");
		printf ("-Fwasm <funcname> causes FTEQCC to dump all asm to qc.asm\n");
		printf ("-O0 to disable optimisations\n");
		printf ("-O1 to optimise for size\n");
		printf ("-O2 to optimise more - some behaviours may change\n");
		printf ("-O3 to optimise lots - experimental or non-future-proof\n");
		printf ("-Oname to enable an optimisation\n");
		printf ("-Ono-name to disable optimisations\n");
		printf ("-Kkeyword to activate keyword\n");
		printf ("-Kno-keyword to disable keyword\n");
		printf ("-Wall to give a stupid number of warnings\n");
		printf ("-Ttarget to set a output format\n");
		printf ("-Fautoproto to enable automatic prototyping\n");
		printf ("-Fsubscope to make locals specific to their subscope\n");

		qcc_compileactive = false;
		return true;
	}

	if (flag_caseinsensitive)
	{
		printf("Compiling without case sensitivity\n");
		pHash_Get = &Hash_GetInsensitive;
		pHash_GetNext = &Hash_GetNextInsensitive;
		pHash_Add = &Hash_AddInsensitive;
		pHash_RemoveData = &Hash_RemoveDataInsensitive;
	}

	if (*qccmsourcedir)
		printf ("Source directory: %s\n", qccmsourcedir);

	QCC_InitData ();

	QCC_PR_BeginCompilation ((void *)qccHunkAlloc (0x100000), 0x100000);

	QCC_PR_ClearGrabMacros (false);

	qccmsrc = NULL;
	if (!numsourcefiles)
	{
		int i;
		for (i = 1;i<myargc;i++)
		{
			if (*myargv[i] == '-')
				break;

			if (!qccmsrc)
			{
				qccmsrc = qccHunkAlloc(8192);
				QC_strlcpy(qccmsrc, "progs.dat\n", 8192);
			}
			QC_strlcat(qccmsrc, myargv[i], 8192);
			QC_strlcat(qccmsrc, "\n", 8192);
		}
	}

	if (qccmsrc)
		;
	else if (flag_acc && !numsourcefiles)
	{
		if (!QCC_FindQCFiles(qccmsourcedir))
			QCC_Error (ERR_COULDNTOPENFILE, "Couldn't find any qc files.");
	}
	else
	{
		if (!numsourcefiles)
		{
			p = QCC_CheckParm ("-qc");
			if (p && p < argc-1 )
				sprintf (qccmprogsdat, "%s", argv[p+1]);
			else
			{	//look for a preprogs.src... :o)
				sprintf (qccmprogsdat, "%spreprogs.src", qccmsourcedir);
				if (externs->FileSize(qccmprogsdat) <= 0)
					sprintf (qccmprogsdat, "progs.src");
				else
					sprintf (qccmprogsdat, "preprogs.src");
			}

			numsourcefiles = 0;
			strcpy(sourcefileslist[numsourcefiles++], qccmprogsdat);
			currentsourcefile = 0;
		}
		else if (currentsourcefile == numsourcefiles)
		{
			//no more.
			qcc_compileactive = false;
			numsourcefiles = 0;
			currentsourcefile = 0;
			return true;
		}

		if (currentsourcefile)
			printf("-------------------------------------\n");
		else
			printf("%s\n", QCC_VersionString());

		sprintf (qccmprogsdat, "%s%s", qccmsourcedir, sourcefileslist[currentsourcefile++]);
		printf ("Source file: %s\n", qccmprogsdat);

		QC_strlcpy(compilingrootfile, qccmprogsdat, sizeof(compilingrootfile));
		if (QCC_LoadFile (qccmprogsdat, (void *)&qccmsrc) == -1)
		{
			return true;
		}
	}

#ifdef WRITEASM
	if (writeasm)
	{
		asmfile = fopen("qc.asm", "wb");
		if (!asmfile)
			QCC_Error (ERR_INTERNAL, "Couldn't open file for asm output.");
	}
	asmfilebegun = !!asmfile;
#endif

	newstylesource = false;
	pr_file_p = QCC_COM_Parse(qccmsrc);

	if (QCC_CheckParm ("-qc"))
	{
		strcpy(destfile, qccmprogsdat);
		StripExtension(destfile);
		strcat(destfile, ".qco");

		p = QCC_CheckParm ("-o");
		if (!p || p >= argc-1 || argv[p+1][0] == '-')
		if (p && p < argc-1 )
			sprintf (destfile, "%s%s", qccmsourcedir, argv[p+1]);
		goto newstyle;
	}

	compilingfile = qccmprogsdat;
	if (*qcc_token == '#')
	{
		void StartNewStyleCompile(void);
newstyle:
		newstylesource = true;
		originalqccmsrc = qccmsrc;
		StartNewStyleCompile();
		return true;
	}

	pr_file_p = qccmsrc;
	QCC_PR_LexWhitespace(false);
	qccmsrc = pr_file_p;

	s = qccmsrc;
	pr_file_p = qccmsrc;
	QCC_PR_SimpleGetToken ();
	strcpy(qcc_token, pr_token);
	qccmsrc = pr_file_p;

	if (!qccmsrc)
		QCC_Error (ERR_NOOUTPUT, "No destination filename.  qcc -help for info.");

	QCC_GenerateRelativePath(destfile, sizeof(destfile), qccmprogsdat, qcc_token);

	p = QCC_CheckParm ("-o");
	if (p > 0 && p < argc-1 && argv[p+1][0] != '-')
		sprintf (destfile, "%s", argv[p+1]);

	if (flag_filetimes)
	{
		struct stat s, os;
		pbool modified = false;

		if (stat(destfile, &os) != -1)
		{
			while ((pr_file_p=QCC_COM_Parse(pr_file_p)))
			{
				if (stat(qcc_token, &s) == -1 || s.st_mtime > os.st_mtime)
				{
					printf("%s changed\n", qcc_token);
					modified = true;
					break;
				}
			}
			if (!modified)
			{
				printf("No changes\n");
				qcc_compileactive = false;
				return true;
			}
			else
			{
				pr_file_p = qccmsrc;
			}
		}
	}

	printf ("outputfile: %s\n", destfile);

	pr_dumpasm = false;

	currentchunk = NULL;

	originalqccmsrc = qccmsrc;
	return true;
}

void new_QCC_ContinueCompile(void);
//called between exe frames - won't loose net connection (is the theory)...
void QCC_ContinueCompile(void)
{
	currentchunk = NULL;
	if (!qcc_compileactive)
		//HEY!
		return;

	if (newstylesource)
	{
		char *ofp = pr_file_p;
		do
		{
			new_QCC_ContinueCompile();
		} while(currentchunk);	//while parsing through preprocessor, make sure nothing gets hurt.
		if (ofp == pr_file_p && qcc_compileactive && pr_token_type != tt_eof)
			QCC_Error (ERR_INTERNAL, "Syntax error\n");

		return;
	}

	pr_file_p = qccmsrc;
	s_filen = "";
	s_filed = 0;
	pr_source_line = 0;
	QCC_PR_LexWhitespace(false);
	qccmsrc = pr_file_p;

	qccmsrc = QCC_COM_Parse(qccmsrc);
	if (!qccmsrc)
	{
		if (parseonly)
		{
			qcc_compileactive = false;
			sourcefilesdefs[currentsourcefile] = qccpersisthunk?pr.def_head.next:NULL;
			sourcefilesnumdefs = currentsourcefile+1;
		}
		else
		{
			if (autoprototype)
			{
				qccmsrc = originalqccmsrc;
				autoprototyped = autoprototype;
				QCC_SetDefaultProperties();
				autoprototype = false;
				return;
			}
			QCC_FinishCompile();
		}
		PostCompile();

		if (currentsourcefile < numsourcefiles)
		{
			if (!QCC_main(myargc, myargv))
				return;
		}
		else
		{
			qcc_compileactive = false;
			numsourcefiles = 0;
			currentsourcefile = 0;
		}
		return;
	}

	if(setjmp(pr_parse_abort))
	{
		if (++pr_error_count > MAX_ERRORS)
			QCC_Error (ERR_PARSEERRORS, "Errors have occured\n");
		return;	//just try move onto the next file, gather errors.
	}
	else
		QCC_FindBestInclude(qcc_token, compilingrootfile, 2);
/*
	{
		int includepath = 0;
		while(1)
		{
			if (includepath)
			{
				if (includepath > MAXINCLUDEDIRS || !*qccincludedir[includepath-1])
				{
					QCC_GenerateRelativePath(qccmfilename, sizeof(qccmfilename), compilingrootfile, qcc_token);
					break;
				}

				currentfile = qccincludedir[includepath-1];
			}

			QCC_Canonicalize(qccmfilename, sizeof(fullname), qcc_token, compilingrootfile);

			{
				extern progfuncs_t *qccprogfuncs;
				if (qccprogfuncs->funcs.parms->FileSize(qccmfilename) == -1)
				{
					includepath++;
					continue;
				}
			}
			break;
		}
	}

	QCC_GenerateRelativePath(qccmfilename, sizeof(qccmfilename), compilingrootfile, qcc_token);
	if (autoprototype)
		printf ("prototyping %s\n", qccmfilename);
	else
	{
		printf ("compiling %s\n", qccmfilename);
	}

	QCC_LoadFile (qccmfilename, (void *)&qccmsrc2);

	if (!QCC_PR_CompileFile (qccmsrc2, qccmfilename) )
		QCC_Error (ERR_PARSEERRORS, "Errors have occured\n");
	*/
}
void QCC_FinishCompile(void)
{
	pbool donesomething;
	int crc;
//	int p;
	currentchunk = NULL;

	if (setjmp(pr_parse_abort))
		QCC_Error(ERR_INTERNAL, "");

	s_filen = "";
	s_filed = 0;
	pr_source_line = 0;

	if (!QCC_PR_FinishCompilation ())
	{
		QCC_Error (ERR_PARSEERRORS, "compilation errors");
	}

/*	p = QCC_CheckParm ("-asm");
	if (p)
	{
		for (p++ ; p<myargc ; p++)
		{
			if (myargv[p][0] == '-')
				break;
			QCC_PrintFunction (myargv[p]);
		}
	}*/

	/*p = QCC_CheckParm ("-ofs");
	if (p)
	{
		for (p++ ; p<myargc ; p++)
		{
			if (myargv[p][0] == '-')
				break;
			QCC_PrintOfs (atoi(myargv[p]));
		}
	}*/

// write progdefs.h
	crc = QCC_PR_WriteProgdefs ("progdefs.h");

	if (pr_error_count)
		QCC_Error (ERR_PARSEERRORS, "compilation errors");

// write data file
	donesomething = QCC_WriteData (crc);

// regenerate bmodels if -bspmodels
	QCC_BspModels ();

// report / copy the data files
	QCC_CopyFiles ();

	sourcefilesdefs[currentsourcefile] = qccpersisthunk?pr.def_head.next:NULL;
	sourcefilesnumdefs = currentsourcefile+1;

	if (donesomething)
	{
		if (verbose)
		{
			printf ("Compile Complete\n\n");

			if (optres_shortenifnots)
				printf("optres_shortenifnots %i\n", optres_shortenifnots);
			if (optres_overlaptemps)
				printf("optres_overlaptemps %i\n", optres_overlaptemps);
			if (optres_noduplicatestrings)
				printf("optres_noduplicatestrings %i\n", optres_noduplicatestrings);
			if (optres_constantarithmatic)
				printf("optres_constantarithmatic %i\n", optres_constantarithmatic);
			if (optres_nonvec_parms)
				printf("optres_nonvec_parms %i\n", optres_nonvec_parms);
			if (optres_constant_names)
				printf("optres_constant_names %i\n", optres_constant_names);
			if (optres_constant_names_strings)
				printf("optres_constant_names_strings %i\n", optres_constant_names_strings);
			if (optres_precache_file)
				printf("optres_precache_file %i\n", optres_precache_file);
			if (optres_filenames)
				printf("optres_filenames %i\n", optres_filenames);
			if (optres_assignments)
				printf("optres_assignments %i\n", optres_assignments);
			if (optres_unreferenced)
				printf("optres_unreferenced %i\n", optres_unreferenced);
			if (optres_locals)
				printf("optres_locals %i\n", optres_locals);
			if (optres_function_names)
				printf("optres_function_names %i\n", optres_function_names);
			if (optres_dupconstdefs)
				printf("optres_dupconstdefs %i\n", optres_dupconstdefs);
			if (optres_return_only)
				printf("optres_return_only %i\n", optres_return_only);
			if (optres_compound_jumps)
				printf("optres_compound_jumps %i\n", optres_compound_jumps);
		//	if (optres_comexprremoval)
		//		printf("optres_comexprremoval %i\n", optres_comexprremoval);
			if (optres_stripfunctions)
				printf("optres_stripfunctions %i\n", optres_stripfunctions);
			if (optres_locals_overlapping)
				printf("optres_locals_overlapping %i\n", optres_locals_overlapping);
			if (optres_logicops)
				printf("optres_logicops %i\n", optres_logicops);
			if (optres_inlines)
				printf("optres_inlines %i\n", optres_inlines);


			if (optres_test1)
				printf("optres_test1 %i\n", optres_test1);
			if (optres_test2)
				printf("optres_test2 %i\n", optres_test2);

			printf("numtemps %u\n", (unsigned)tempsused);
		}
		if (!flag_msvcstyle)
			printf("Done. %i warnings\n", pr_warning_count);
	}

	qcc_compileactive = false;
}





extern char		*pr_file_p;
extern int			pr_source_line;
void QCC_PR_ParseDefs (char *classname);





void StartNewStyleCompile(void)
{
	char *tmp;
	if (setjmp(pr_parse_abort))
	{
		if (++pr_error_count > MAX_ERRORS)
			return;
		if (setjmp(pr_parse_abort))
			return;

		QCC_PR_SkipToSemicolon ();
		if (pr_token_type == tt_eof)
			return;
	}

	compilingfile = qccmprogsdat;

	s_filen = tmp = qccHunkAlloc(strlen(compilingfile)+1);
	strcpy(tmp, compilingfile);
	if (opt_filenames)
	{
		optres_filenames += strlen(compilingfile)+1;
		s_filed = 0;
	}
	else
		s_filed = QCC_CopyString (compilingfile);

	pr_file_p = qccmsrc;

	pr_source_line = 0;

	QCC_PR_NewLine (false);

	QCC_PR_Lex ();	// read first token
}
void new_QCC_ContinueCompile(void)
{
	if (setjmp(pr_parse_abort))
	{
//		if (pr_error_count != 0)
		{
			QCC_Error (ERR_PARSEERRORS, "Errors have occured");
			return;
		}
		QCC_PR_SkipToSemicolon ();
		if (pr_token_type == tt_eof)
			return;
	}

	if (pr_token_type == tt_eof)
	{
		if (pr_error_count)
			QCC_Error (ERR_PARSEERRORS, "Errors have occured");

		if (autoprototype && !parseonly)
		{
			char *tmp;
			qccmsrc = originalqccmsrc;

			s_filen = tmp = qccHunkAlloc(strlen(compilingfile)+1);
			strcpy(tmp, compilingfile);
			if (opt_filenames)
			{
				optres_filenames += strlen(compilingfile)+1;
				s_filed = 0;
			}
			else
				s_filed = QCC_CopyString (compilingfile);
			pr_file_p = qccmsrc;

			autoprototyped = autoprototype;
			QCC_SetDefaultProperties();
			autoprototype = false;
			QCC_PR_NewLine(false);
			QCC_PR_Lex();
			return;
		}
		else
		{
			if (!parseonly)
				QCC_FinishCompile();
			else
			{
				sourcefilesdefs[currentsourcefile] = qccpersisthunk?pr.def_head.next:NULL;
				sourcefilesnumdefs = currentsourcefile+1;
			}
			PostCompile();
			if (!QCC_main(myargc, myargv))
			{
				qcc_compileactive = false;
				return;
			}
			return;
		}
	}

	pr_scope = NULL;	// outside all functions

	QCC_PR_ParseDefs (NULL);
}

/*void new_QCC_ContinueCompile(void)
{
	char *s, *s2;
	if (!qcc_compileactive)
		//HEY!
		return;

// compile all the files

		qccmsrc = QCC_COM_Parse(qccmsrc);
		if (!qccmsrc)
		{
			QCC_FinishCompile();
			return;
		}
		s = qcc_token;

		strcpy (qccmfilename, qccmsourcedir);
		while(1)
		{
			if (!strncmp(s, "..\\", 3))
			{
				s2 = qccmfilename + strlen(qccmfilename)-2;
				while (s2>=qccmfilename)
				{
					if (*s2 == '/' || *s2 == '\\')
					{
						s2[1] = '\0';
						break;
					}
					s2--;
				}
				s+=3;
				continue;
			}
			if (!strncmp(s, ".\\", 2))
			{
				s+=2;
				continue;
			}

			break;
		}
//		strcat (qccmfilename, s);
//		printf ("compiling %s\n", qccmfilename);
//		QCC_LoadFile (qccmfilename, (void *)&qccmsrc2);

//		if (!new_QCC_PR_CompileFile (qccmsrc2, qccmfilename) )
//			QCC_Error ("Errors have occured\n");


		{

			if (!pr.memory)
				QCC_Error ("PR_CompileFile: Didn't clear");

			QCC_PR_ClearGrabMacros ();	// clear the frame macros

			compilingfile = filename;

			pr_file_p = qccmsrc2;
			s_file = QCC_CopyString (filename);

			pr_source_line = 0;

			QCC_PR_NewLine ();

			QCC_PR_Lex ();	// read first token

			while (pr_token_type != tt_eof)
			{
				if (setjmp(pr_parse_abort))
				{
					if (++pr_error_count > MAX_ERRORS)
						return false;
					QCC_PR_SkipToSemicolon ();
					if (pr_token_type == tt_eof)
						return false;
				}

				pr_scope = NULL;	// outside all functions

				QCC_PR_ParseDefs ();
			}
		}
	return (pr_error_count == 0);

}*/


#endif
