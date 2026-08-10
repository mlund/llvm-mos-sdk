# AWK script to generate the contents of this directory.

BEGIN {
  file = "imag-regs.ld"
  print "/* GENERATED FILE -- DO NOT MANUALLY EDIT. */" > file;
  print "/* Note: The odd regs must immediately follow the even regs" > file;
  print " * for even/odd pairs to work as pointers, but the even regs" > file;
  print " * are otherwise arbitrary unless the Imag32 contract is used. */" > file;
  for (i = 1; i < 32; i++) {
    # The odd regs must immediately follow the even regs to be used as pointers.
    # Outside the Imag32 contract, the even regs are unconstrained.
    if (i % 2 == 0)
      printf "PROVIDE(" > file;
    printf "__rc%s = __rc%s + 1", i, i - 1 > file;
    if (i % 2 == 0)
      printf ")" > file
    printf ";\n" > file
  }
  print "/* Keep the contract dormant until compiler output references it. */" > file;
  print "PROVIDE(__mos_imag32_contiguous = ASSERT(" > file;
  print "  __rc6 == __rc4 + 2 && __rc14 == __rc12 + 2 &&" > file;
  print "  __rc22 == __rc20 + 2 && __rc26 == __rc24 + 2," > file;
  print "  \"Imag32 requires contiguous allocatable register quads.\"));" > file;
}
