# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import subprocess


def configureDoxyfile(input_dir, output_dir):
    with open("Doxyfile.in", "r") as file:
        filedata = file.read()

    filedata = filedata.replace("@DOXYGEN_INPUT_DIR@", input_dir)
    filedata = filedata.replace("@DOXYGEN_OUTPUT_DIR@", output_dir)

    with open("Doxyfile", "w") as file:
        file.write(filedata)


# check if we are running on ReadTheDoc's servers
read_the_docs_build = os.environ.get("READTHEDOCS", None) == "True"

breathe_projects = {}
if read_the_docs_build:
    input_dir = "../input"
    output_dir = "build"
    configureDoxyfile(input_dir, output_dir)
    subprocess.call("doxygen", shell=True)
    breathe_projects["krowkee"] = output_dir + "/xml"
    print("gets here")

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "krowkee"
copyright = "2023, Benjamin W. Priest"
author = "Benjamin W. Priest"
release = "0.1"

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
breathe_default_project = "krowkee"
breathe_default_members = ("members",)
