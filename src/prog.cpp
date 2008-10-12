int main()
{
    __asm (
            "mov eax,10\n"
            "mov ebx,12\n"
            "pushd 66\n"
            "ret 8\n"
          );
    return 0;
}
