/* ---
 * Compile with:
 *   gcc -lcrelude 0_hello_world.c -o hello-world
 * Run with:
 *   ./hello-world Cornelius
 * ---
 */

#define ENTRY_FUNCTION init
#include <crelude/common.h>

ierr init(Arguments args)
{
	string name = STRING("World");

	unless (args.len <= 1)
		name = NTH(args, 1);

	println("Hello %S!", name);
	println("Your name is %zu bytes: %V{0x%hhX}{, }.", name.len, name);

	return OK;
}
