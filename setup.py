import platform
import sys
from distutils.core import Extension, setup


def main():

    include_dirs=["frontpanel"]
    define_macros=[]
    extra_compile_args=[]
    extra_link_args=[]

    if sys.platform == "darwin":
        extra_compile_args=["-std=c++17"]
        if platform.machine() == "arm64":
            extra_link_args=["-lokFrontPanel", "-Lfrontpanel/mac/arm64/", "-Wl,-rpath", "./"]
        else:
            extra_link_args=["-lokFrontPanel", "-Lfrontpanel/mac"]


    if sys.platform == "linux":
        extra_compile_args=["-std=c++17"]
        extra_link_args=["-lokFrontPanel", "-Lfrontpanel/linux", "-Wl,-rpath=./"]

    if sys.platform == "win32":
        define_macros=[('WIN32', '1')]
        extra_compile_args=["/std:c++17"]
        extra_link_args=["/LIBPATH:frontpanel/win/x64"]


    setup(name="py_fp",
            version="1.0.1",
            description="Front Panel Library",
            author="Daniel Turecek",
            author_email="daniel@turecek.de",
            include_package_data=True,
            packages=["py_fp"],
            package_dir={"py_fp": "py_fp"},
            package_data={"py_fp": [
                "__init__.pyi",
                "py.typed",
            ]},
            ext_modules=[
                 Extension(
                    "py_fp",
                    sources=["py_fp/py_fp.cpp",
                             "py_fp/fpdev.cpp" ],
                    include_dirs=include_dirs,
                    define_macros=define_macros,
                    extra_compile_args=extra_compile_args,
                    extra_link_args=extra_link_args,
                )
            ])

if __name__ == "__main__":
    main()
