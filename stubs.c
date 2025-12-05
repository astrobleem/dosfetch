// Manual REGS definition to avoid including dos.h/i86.h and conflicting with int86 declaration
struct WORDREGS {
    unsigned short ax, bx, cx, dx, si, di, cflag, flags;
};
struct BYTEREGS {
    unsigned char al, ah, bl, bh, cl, ch, dl, dh;
};
union REGS {
    struct WORDREGS w;
    struct BYTEREGS h;
};

// Satisfy linker for __acrtused
int _acrtused = 0;

// MS C Stack Checking Helper
// Implemented in stubs_asm.asm



// Absolute value
int __cdecl abs(int n) {
    return (n < 0) ? -n : n;
}

// Declare Watcom's int86 (Watcall convention)
// Watcom usually appends an underscore to C symbols in Watcall? 
// Or maybe it's just "int86_" in the library.
extern int __watcall real_int86(int, union REGS*, union REGS*);
#pragma aux real_int86 "int86_";

// Define int86 as Cdecl (which produces _int86 symbol)
int __cdecl int86(int intr, union REGS *in, union REGS *out) {
    return real_int86(intr, in, out);
}
