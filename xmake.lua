add_rules("mode.debug", "mode.release")
add_requires("libopus latest", "libsamplerate latest", "portaudio latest", "boost latest", "minimp3 latest")
set_defaultmode("debug")
set_languages("c17", "c++20")
add_headerfiles("include/*.hpp")
target("server")
set_kind("binary")
-- add_includedirs("C:\\gstreamer\\1.0\\msvc_x86_64\\include\\gstreamer-1.0",{public = true})
-- add_includedirs("C:\\gstreamer\\1.0\\msvc_x86_64\\include\\glib-2.0",{public = true})
-- add_includedirs("C:\\gstreamer\\1.0\\msvc_x86_64\\lib\\glib-2.0\\include",{public = true})
-- add_includedirs("C:\\gstreamer\\1.0\\msvc_x86_64\\lib\\glib-2.0\\include",{public = true})
-- add_includedirs("C:\\gstreamer\\1.0\\msvc_x86_64\\include",{public = true})
-- add_links("C:\\gstreamer\\1.0\\msvc_x86_64\\lib\\*")
add_files("src/server.cpp")
add_packages("libopus", "libsamplerate", "portaudio", "boost", "minimp3")

target("test")
set_kind("binary")
add_packages("libopus", "libsamplerate", "portaudio", "boost", "minimp3")
add_files("src/tests/test.cpp")

target("client")
set_kind("binary")
add_files("src/client.cpp")
add_packages("libopus", "libsamplerate", "portaudio", "boost", "minimp3")

target("record")
set_kind("binary")
add_files("src/record.cpp")
add_packages("libopus", "libsamplerate", "portaudio", "boost", "minimp3")

target("call")
set_kind("binary")
add_files("src/call.cpp")
add_packages("libopus", "libsamplerate", "portaudio", "boost", "minimp3")

target("relay")
set_kind("binary")
add_files("src/relay.cpp")
add_packages("libopus", "libsamplerate", "portaudio", "boost", "minimp3")
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--
