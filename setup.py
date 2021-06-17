import os
import re
import sys
import platform
import subprocess
import glob

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion
from distutils.command.install_headers import install_headers as install_headers_orig
from setuptools import setup
from distutils.sysconfig import get_python_inc
import site #section 2.7 in https://docs.python.org/3/distutils/setupscript.html

# https://stackoverflow.com/a/287944
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def check_file(f):
    if not os.path.exists(f):
        # colored('hello', 'red')
        # print(colored('hello', 'red'), colored('world', 'green'))
        print(bcolors.FAIL + "Could not find {}".format(f)+bcolors.ENDC)
        print(bcolors.FAIL + "Did you run 'git submodule update --init --recursive'?"+bcolors.ENDC)
        sys.exit(1)



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

        #check if we have all dependencies
        check_file(os.path.join(os.getcwd(), 'deps', 'libigl', 'CMakeLists.txt'))

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = [
                    '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable
                    #   '-GNinja'
                      ]

        # cfg = 'Debug' if self.debug else 'Release'
        # build_args = ['--config', cfg]
        build_args = []

        if platform.system() == "Windows":
            # cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            # cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j4']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        # print ("build temp is ", self.build_temp)

        #find out where do the header file and the shader files get copied into https://stackoverflow.com/questions/14375222/find-python-header-path-from-within-python
        # print("PRINTING-------------------------------------------------")
        # print( get_python_inc() ) #this gives the include dir
        # print( site.USER_BASE )  # data files that are added through data_files=[] get added here for --user instaltions https://docs.python.org/3/distutils/setupscript.html
        # pkg_dir, dist = self.create_dist(headers="dummy_header")
        # dummy_install_headers=install_headers_orig(dist=dist)
        # help(install_headers_orig)
        # dummy_install_headers=install_headers_orig(self.distribution)
        # print( dummy_install_headers.install_dir ) #this gives the include dir
        # cmake_args+=['-DEASYPBR_SHADERS_PATH=' + get_python_inc()+"/easypbr"]
        # cmake_args+=['-DEASYPBR_SHADERS_PATH=' + site.USER_BASE]
        # cmake_args+=['-DDATA_DIR=' + site.USER_BASE]


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
            print("----------------copying in ", dst)
            # dst = os.path.join(get_python_inc(), os.path.dirname(header))
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
    # package_data={
    #   '': ['*.txt'],
    #   '': ['shaders/render/*.glsl'],
    # },
    # include_package_data=True,
    # headers=['shaders/render/compose_frag.glsl', 'include/easy_pbr/LabelMngr.h'],
    # headers=['include/easy_pbr/LabelMngr.h'],
    # headers=find_files('include/easy_pbr/',"", [".h", ".hpp", ".hh", "cuh" ] ) + find_files('shaders/ibl/',"", [".glsl"]) + find_files('shaders/render/',"", [".glsl"]) + find_files('shaders/ssao/',"", [".glsl"] ),
    # data_files=[
    #             ('shaders/ibl/', glob.glob('shaders/ibl/*.glsl') ),
    #             ('shaders/render/', glob.glob('shaders/render/*.glsl') ),
    #             ('shaders/ssao/', glob.glob('shaders/ssao/*.glsl') ),
    #             ('data/fonts/', glob.glob('data/fonts/*.*') )
    #               ],
    # headers=find_headers('shaders/ibl/',"", [".glsl"] ),
    # headers=find_headers('shaders/render/',"", [".glsl"] ),
    # headers=find_headers('shaders/ssao/',"", [".glsl"] ),
    # cmdclass={'install_headers': install_headers},
)

# print("copied headers ", find_files('include/easy_pbr/',"", [".h", ".hpp", ".hh", "cuh" ] ) )
# print("copied headers ", glob.glob('shaders/ssao/*.glsl') )








# #Attempt 2
# import setuptools
# import setuptools.command.install
# from setuptools import setup
# from torch.utils.cpp_extension import CppExtension, BuildExtension, CUDAExtension
# import subprocess
# import os
# import sys
# import torch
# import glob

# BUILD_PATH = os.path.join(os.getcwd(), 'build', 'cpp_build')
# INSTALL_PATH = os.path.join(os.getcwd(), 'build', 'easypbr')

# DEBUG = int(os.environ.get('DEBUG', '0')) == 1

# def build_stillleben():
#     os.makedirs(BUILD_PATH, exist_ok=True)

#     cmd = [
#         'cmake',
#         '-DCMAKE_BUILD_TYPE={}'.format('Debug' if DEBUG else 'RelWithDebInfo'),
#         '-DCMAKE_INSTALL_PREFIX=' + INSTALL_PATH,
#         '-DUSE_RELATIVE_RPATH=ON',
#         '-GNinja',
#         '../..'
#     ]

#     env = os.environ.copy()
#     env['CLICOLOR_FORCE'] = '1'

#     # are we installing inside anaconda?
#     # if so, try to be helpful.
#     if 'CONDA_PREFIX' in env:
#         cmd.append(f'-DEXTRA_RPATH={env["CONDA_PREFIX"]}/lib')

#         if 'CMAKE_PREFIX_PATH' in env:
#             env['CMAKE_PREFIX_PATH'] = env['CONDA_PREFIX'] + ':' + env['CMAKE_PREFIX_PATH']
#         else:
#             env['CMAKE_PREFIX_PATH'] = env['CONDA_PREFIX']

#     if subprocess.call(cmd, cwd=BUILD_PATH, env=env) != 0:
#         print('Failed to run "{}"'.format(' '.join(cmd)))

#     make_cmd = [
#         'ninja',
#         'install',
#     ]

#     if subprocess.call(make_cmd, cwd=BUILD_PATH, env=env) != 0:
#         raise RuntimeError('Failed to call ninja')

# # Build all dependent libraries
# class build_deps(setuptools.Command):
#     user_options = []

#     def initialize_options(self):
#         pass

#     def finalize_options(self):
#         pass

#     def run(self):
#         # Check if you remembered to check out submodules

#         # def check_file(f):
#         #     if not os.path.exists(f):
#         #         print("Could not find {}".format(f))
#         #         print("Did you run 'git submodule update --init --recursive'?")
#         #         sys.exit(1)

#         # check_file(os.path.join(os.getcwd(), 'contrib', 'corrade', 'CMakeLists.txt'))
#         # check_file(os.path.join(os.getcwd(), 'contrib', 'magnum', 'CMakeLists.txt'))
#         # check_file(os.path.join(os.getcwd(), 'contrib', 'magnum-plugins', 'CMakeLists.txt'))

#         build_stillleben()

# class install(setuptools.command.install.install):
#     def run(self):
#         if not self.skip_build:
#             self.run_command('build_deps')

#         setuptools.command.install.install.run(self)

# def make_relative_rpath(path):
#     return '-Wl,-rpath,$ORIGIN/' + path

# cmdclass = {
#     'build_deps': build_deps,
#     'install': install,
# }

# MAGNUM_SUFFIX = '-d' if DEBUG else ''

# setuptools.setup(
#     name='easypbr',
#     version='1.0.0',
#     cmdclass=cmdclass,
#     # packages=[''],
#     package_dir={'':'.'},
#     package_data={
#         '.': [
#             '*.so',
#             'lib/*.so*',
#             'lib/magnum{}/importers/*'.format(MAGNUM_SUFFIX),
#             'lib/magnum{}/imageconverters/*'.format(MAGNUM_SUFFIX),
#         ]
#     },
#     ext_modules=[],
#     options={
#         'build': {
#             'build_base': 'python/py_build'
#         },
#         'sdist': {
#             'dist_dir': 'python/dist'
#         },
#     },
# )
