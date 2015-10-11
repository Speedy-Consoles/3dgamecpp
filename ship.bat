MKDIR %package_directory%
DEL %package_directory% /s /q

XCOPY msvc\bin\%platform_id%\Release\client.exe %package_directory% /y
XCOPY msvc\bin\%platform_id%\Release\server.exe %package_directory% /y

XCOPY fonts %package_directory%\fonts /s /e /i /y
XCOPY img %package_directory%\img /s /e /i /y
XCOPY res %package_directory%\res /s /e /i /y
XCOPY shaders %package_directory%\shaders /s /e /i /y
XCOPY sounds %package_directory%\sounds /s /e /i /y

XCOPY block_ids.yml %package_directory% /y
XCOPY sounds_*.yml %package_directory% /y
XCOPY textures_*.txt %package_directory% /y
XCOPY logging.conf %package_directory% /y
XCOPY logging_srv.conf %package_directory% /y

XCOPY deps\lib-%platform_id%-Release\glew32.dll %package_directory% /y
XCOPY deps\lib-%platform_id%-Release\SDL2.dll %package_directory% /y
XCOPY deps\lib-%platform_id%-Release\SDL2_image.dll %package_directory% /y
XCOPY deps\lib-%platform_id%-Release\SDL2_mixer.dll %package_directory% /y

XCOPY deps\lib-%platform_id%\ftgl.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libFLAC-8.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libogg-0.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libvorbis-0.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libvorbisfile-3.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\smpeg2.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libjpeg-9.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libpng16-16.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libtiff-5.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\libwebp-4.dll %package_directory% /y
XCOPY deps\lib-%platform_id%\zlib1.dll %package_directory% /y
