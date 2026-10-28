/*
** Fixture covering every symbol class ft_nm has to classify like GNU nm:
**   T/t text, D/d data, B/b bss, R/r rodata, A absolute, C common,
**   W weak function, V weak object, w/v undefined weak (function / object),
**   i GNU ifunc, U undefined, TLS (D/B), custom sections, constructors.
** Compiled by tests/run_tests.sh as .o, executable and shared object
** (and with -m32 when the toolchain has it).
*/
#include <stddef.h>

int					strong_data = 1;
int					bss_data;
static int			static_data = 2;
static int			static_bss;
const int			const_data = 3;
const char			ro_string[] = "hello";
static const char	static_ro[] = "x";

__attribute__((weak)) int	weak_data = 7;
__attribute__((weak)) int	weak_bss;
__attribute__((weak)) void	weak_fn(void) {}
extern __attribute__((weak)) int	undef_weak_data;
extern __attribute__((weak)) void	undef_weak_fn(void);

__thread int		tls_data = 3;
__thread int		tls_bss;

__attribute__((section("mysec"))) int			custom_section = 4;
__attribute__((section(".mydata"), used)) int	custom_dot_section = 5;

int					common_var __attribute__((common));

static void			ctor(void) __attribute__((constructor));
static void			ctor(void) {}

void				*resolver_sym(void) { return (void *)ctor; }
void				ifunc_fn(void) __attribute__((ifunc("resolver_sym")));

__asm__(".globl absolute_sym\n.set absolute_sym, 0x1234\n");

int main(void)
{
	return (undef_weak_data + (undef_weak_fn != NULL) + tls_data + tls_bss
		+ static_data + static_bss + const_data + (int)ro_string[0]
		+ (int)static_ro[0] + strong_data + bss_data);
}
