import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="easypbr",
    version="0.0.1",
    author="Radu Alexandru Rosu",
    author_email="rosu@ais.uni-bonn.de",
    description="Physically based rendering made easy",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/RaduAlexandru/easy_pbr",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)