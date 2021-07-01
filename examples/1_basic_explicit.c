/*
 * Compile with:
 *   gcc -lcrelude ./1_basic_explicit -o basic-explicit
 * Then run:
 *   ./basic-explicit a b c
 */

#include <crelude/common.h>
#include <crelude/utf.h>

newarray(CStringArray, const byte *);
newarray(StringArray, string);

ierr main(i32 argc, const byte **argv)
{
	StringArray args = AMAP(StringArray, to_string,
		ACOLLECT(CStringArray, argc, argv));

	string name = STRING("World");
	println("Hello, %S!", name);

	foreach (arg, args)
		println("Arg passed: %S.", *arg);

	println("In summary, that was: %D{'%S'}{, }.", args);

	StringBuilder with_dots = AMAKE(byte, 0);
	extend(&with_dots, &name, sizeof(byte));
	extend(&with_dots, &STR("..."), sizeof(byte));

	println("Goodbye, %D%c{ }", with_dots);

	return OK;
}
