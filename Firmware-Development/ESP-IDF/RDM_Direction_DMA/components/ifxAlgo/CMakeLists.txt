idf_component_register(SRCS "2DMTI.c"
                            "FFT.c"
                            "PreprocessedFFT.c"
                            "DBSCAN.c"
                            "OSCFAR.c"
                            "Window.c"
                            "MTI.c"
                            "Signal.c"
                    INCLUDE_DIRS "include" REQUIRES ifxBase muFFT)


