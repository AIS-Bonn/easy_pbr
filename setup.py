import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion
from distutils.command.install_headers import install_headers as install_headers_orig
from setuptools import setup
from distutils.sysconfig import get_python_inc

BUILD_PATH = os.path.join(os.getcwd(), 'python', 'cpp_build')
INSTALL_PATH = os.path.join(os.getcwd(), 'python', 'stillleben')


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
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      '-GNinja'
                      ]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j8']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        # print ("build temp is ", self.build_temp)

        #find out where do the header file and the shader files get copied into https://stackoverflow.com/questions/14375222/find-python-header-path-from-within-python
        print("PRINTING-------------------------------------------------")
        print( get_python_inc() ) #this gives the include dir
        cmake_args+=['-DEASYPBR_SHADERS_PATH=' + get_python_inc()+"/easypbr"]


        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)
        # subprocess.check_call(['make', 'install'], cwd=self.build_temp)


with open("README.md", "r") as fh:
    long_description = fh.read()

#install headers while retaining the structure of the tree folder https://stackoverflow.com/a/50114715
class install_headers(install_headers_orig):

    def run(self):
        headers = self.distribution.headers or []
        for header in headers:
            dst = os.path.join(self.install_dir, os.path.dirname(header))
            self.mkpath(dst)
            (out, _) = self.copy_file(header, dst)
            self.outfiles.append(out)

def has_any_extension(filename, extensions):
    for each in extensions:
        if filename.endswith(each):
            return True
    return False

# https://stackoverflow.com/a/41031566
def find_files(directory, strip, extensions):
    """
    Using glob patterns in ``package_data`` that matches a directory can
    result in setuptools trying to install that directory as a file and
    the installation to fail.

    This function walks over the contents of *directory* and returns a list
    of only filenames found. The filenames will be stripped of the *strip*
    directory part.

    It only takes file that have a certain extension
    """

    result = []
    for root, dirs, files in os.walk(directory):
        for filename in files:
            # if filename.endswith('.h') or filename.endswith('.hpp') or filename.endswith('.cuh'):
            if has_any_extension(filename, extensions):
                # print("filename", filename)
                filename = os.path.join(root, filename)
                result.append(os.path.relpath(filename, strip))

    # print("result of find_files is ", result)

    return result
    # return 'include/easy_pbr/LabelMngr.h'

setup(
    name='easypbr',
    version='1.0.0',
    author="Radu Alexandru Rosu",
    author_email="rosu@ais.uni-bonn.de",
    description="Physically based rendering made easy",
    long_description=long_description,
    ext_modules=[CMakeExtension('easypbr')],
    # cmdclass=dict(build_ext=CMakeBuild),
    cmdclass={ 'build_ext':CMakeBuild,
               'install_headers': install_headers,
    },
    zip_safe=False,
    # packages=['.'],
    # package_dir={'': '.'},
    # package_data={'': ['*.so']},
    # package_data={
    #   'easypbr': ['*.txt'],
    #   'easypbr': ['shaders/render/*.glsl'],
    # },
    # include_package_data=True,
    # headers=['shaders/render/compose_frag.glsl', 'include/easy_pbr/LabelMngr.h'],
    # headers=['include/easy_pbr/LabelMngr.h'],
    headers=find_files('include/easy_pbr/',"", [".h", ".hpp", ".hh", "cuh" ] ) + find_files('shaders/ibl/',"", [".glsl"]) + find_files('shaders/render/',"", [".glsl"]) + find_files('shaders/ssao/',"", [".glsl"] ),
    # headers=find_headers('shaders/ibl/',"", [".glsl"] ),
    # headers=find_headers('shaders/render/',"", [".glsl"] ),
    # headers=find_headers('shaders/ssao/',"", [".glsl"] ),
    # cmdclass={'install_headers': install_headers},
)

print("copied headers ", find_files('include/easy_pbr/',"", [".h", ".hpp", ".hh", "cuh" ] ) )
