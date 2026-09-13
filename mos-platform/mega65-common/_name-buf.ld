/* Hyppo copies a filename only into a page-aligned buffer below $7F00. It
 * accepts page $00 too; we do not, because a name there would overwrite the
 * imaginary registers. INCLUDE after the platform PROVIDEs its default. */
ASSERT((__mega65_h_name_buf & 0xFF) == 0 && __mega65_h_name_buf >= 0x0100 &&
       __mega65_h_name_buf < 0x7F00,
       "__mega65_h_name_buf must be a page in $0100-$7E00")
