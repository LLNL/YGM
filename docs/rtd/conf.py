# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "ygm"
copyright = "2023, LLNL YGM team"
author = "LLNL YGM team"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["breathe"]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]

# Breathe Configuration
breathe_default_project = "ygm"


# -- Run Doxygen -------------------------------------------------
import os
import subprocess


# def run_cmake_build(build_dir):
#     # -- The following commands are executed in the docs/rtd directory -- #
#     try:
#         subprocess.run(["mkdir", build_dir], check=True)
#         # save the current working directory
#         wd = os.getcwd()
#         os.chdir(build_dir)
#         # Use the CMakeLists.txt file in the root directory
#         subprocess.run(["cmake", "../../../", "-DYGM_RTD_ONLY=ON"], check=True)
#         # Generate the doxygen xml files
#         subprocess.run(["make", "doxygen"], check=True)
#         # Back to the original working directory
#         os.chdir(wd)

#     except subprocess.CalledProcessError as e:
#         print(f"Failed to run cmake build: {e}")
#         raise


def configureDoxyfile(input_dir, output_dir):
    with open("../Doxyfile.in", "r") as file:
        filedata = file.read()

    filedata = filedata.replace("@DOXYGEN_INPUT_DIR@", input_dir)
    filedata = filedata.replace("@DOXYGEN_OUTPUT_DIR@", output_dir)

    with open("../Doxyfile", "w") as file:
        file.write(filedata)


# Check if we're running on Read the Docs' servers
read_the_docs_build = os.environ.get("READTHEDOCS", None) == "True"

if read_the_docs_build:
    # build_dir = "build-doc"
    # run_cmake_build(build_dir)
    input_dir = "../../include"
    output_dir = "build-doc"
    configureDoxyfile(input_dir, output_dir)
    subprocess.call("doxygen", shell=True)
    # Breathe Configuration
    # Specify the directory where the doxygen xml files are generated,
    # which is determined by the CMakeLists.txt file in docs/
    breathe_projects = {"ygm": output_dir + "/xml"}
