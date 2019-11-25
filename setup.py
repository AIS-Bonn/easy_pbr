import setuptools
import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)



with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name='easypbr',
    version='1.0.0',
    author="Radu Alexandru Rosu",
    author_email="rosu@ais.uni-bonn.de",
    description="Physically based rendering made easy",
    long_description=long_description,
    ext_modules=[CMakeExtension('easypbr')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,



    # name="easypbr",
    # version="0.0.1",
    # author="Radu Alexandru Rosu",
    # author_email="rosu@ais.uni-bonn.de",
    # description="Physically based rendering made easy",
    # long_description=long_description,
    # long_description_content_type="text/markdown",
    # url="https://github.com/RaduAlexandru/easy_pbr",
    # # packages=setuptools.find_packages(),
    # classifiers=[
    #     "Programming Language :: Python :: 3",
    #     "License :: OSI Approved :: MIT License",
    #     "Operating System :: OS Independent",
    # ],
    # python_requires='>=3.6',
    # packages=setuptools.find_packages('src'),
    # # tell setuptools that all packages will be under the 'src' directory
    # # and nowhere else
    # package_dir={'':'.'},
    # # add an extension module named 'python_cpp_example' to the package 
    # # 'python_cpp_example'
    # ext_modules=[CMakeExtension('easypbr/easypbr')],
    # # add custom build_ext command
    # cmdclass=dict(build_ext=CMakeBuild),
    # zip_safe=False,
    # #https://github.com/meshy/pythonwheels/issues/46
    # # entry_points = {
    #     # 'console_scripts': [''],
    # # }
)