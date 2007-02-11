use ExtUtils::MakeMaker;

        $lib_path = '-lcwiimote -lbluetooth ';


        # See lib/ExtUtils/MakeMaker.pm for details of how to influence
        # the contents of the Makefile that is written.
        WriteMakefile(
            NAME         => 'Linux::Input::Wiimote',
            VERSION_FROM => 'Wiimote.pm', # finds $VERSION
            LIBS         => [$lib_path],   # e.g., '-lm'
            DEFINE       => '-D_ENABLE_TILT -D_ENABLE_FORCE',     # e.g., '-DHAVE_SOMETHING'
            INC          => '-I./src',     # e.g., '-I/usr/include/other'
        );
            #LIBS         => [''],   # e.g., '-lm'
