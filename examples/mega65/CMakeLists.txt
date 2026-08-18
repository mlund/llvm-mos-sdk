add_executable(viciv_test.prg viciv_test.c)
install_example(viciv_test.prg)

add_executable(plasma.prg plasma.cc)
install_example(plasma.prg)

add_executable(vertical_raster.prg vertical_raster.cc)
install_example(vertical_raster.prg)

add_executable(simple_dma.prg simple_dma.cc)
install_example(simple_dma.prg)

add_executable(dma_audio.prg dma_audio.c)
install_example(dma_audio.prg)
install(FILES drums.s8 TYPE BIN)

add_executable(mandelbrot.prg mandelbrot.cc)
install_example(mandelbrot.prg)

add_executable(mandelbrot_fcm.prg mandelbrot_fcm.cc)
install_example(mandelbrot_fcm.prg)

add_executable(vector_logo.prg vector_logo.cc)
install_example(vector_logo.prg)

add_executable(fire_fcm.prg fire_fcm.cc)
install_example(fire_fcm.prg)

add_executable(logo_fire.prg logo_fire.cc)
install_example(logo_fire.prg)

add_executable(joytest.prg joytest.cc)
install_example(joytest.prg)

add_executable(rrb_tile.prg rrb_tile.cc)
install_example(rrb_tile.prg)

