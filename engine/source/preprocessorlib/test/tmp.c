/**
 * This is a silly test script for testing the preprocessor functionality using 
 * pp_test.
 */

#include "../../../testing/data.MYMOD/scripts/blinkd.c"
#import "data/scripts/somefile.c"

// this is a comment and it is \
continued on the next line using a backslash escape.

#define REVERSE __reverse__
//#undef \
REVERSE

// func is a function-style macro; notfunc is not
#define func(x) [x]
#define notfunc (x)

func(42)     // correct output: [42]
notfunc(42)  // correct output: (x)(42)

// make sure nested/multi-layered macro expansion is working
#define PSEUDO1 REVERSE
#define PSEUDO2 func
PSEUDO1      // correct output: __reverse__
PSEUDO2(12)  // correct output: [12]

/* make sure function macros started inside of another macro can be completed by 
 * subsequent text (this is a corner case) */
#define partial func(17
partial)     // correct output: [17]

// make sure the "##" concatenation operator works
#define concat(a, b) a ## b
concat(alpha , beta) // correct output: alphabeta

// make sure #undef works for non-function macros
#undef notfunc
#ifdef notfunc
#error notfunc is still defined; #undef seems to be broken for non-function macros
#endif

// make sure #undef works for function macros
#undef func
#ifdef func
#error func is still defined; #undef seems to be broken for function macros
#endif

// make sure the "#" operator works inside of function-style #defines
#define stringify(x) # x
stringify("identifier" + num + 3)

// make sure null directives are "supported" (i.e. ignored)
#

// make sure "unexpected newline" errors are readable
//#include

#ifdef REVERSE
#define BLINK(oof, rab, oot) knilb oot rab oof
#define HEALTH \
ph()
#warning REVERSE is defined
#else
#define BLINK(foo,bar,\
too) foo bar too blink
#define \
HEALTH hp++
#warning REVERSE is not defined
#endif

void BLINK(foo, bar, ctx->tokens)
{// Blink effect script
    void self = getlocalvar("self"); //Get calling entity.
    void HEALTH = getentityproperty(self, "health"); \

    if (HEALTH > 0){
		changeentityproperty(self, "colourmap", 1); /* */
		changeentityproperty(self, "maptime", 20 + openborvariant("elapsed_time"));
    }
}

#define eof This is the end of the file!
