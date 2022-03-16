# How to Contribute

`krowkee` is an open source project.
Our team welcomes contributions from collaborators in the form of raising issues 
as well as code contributions including hotfixes, code improvements, and new
features.

`krowkee` is distributed under the terms of the 
[MIT license](https://github.com/LLNL/krowkee/blob/main/LICENSE-MIT). 
All new contributions must be made under this license.

If you identify a problem such as a bug or awkward or confusing code, or require
a new feature, please feel free to start a thread on our 
[issue tracker](https://github.com/LLNL/krowkee/issues).
Please first review the existing issues prior to avoid duplicate issues. 
 
If you plan on contributing to `krowkee`, please review the 
[issue tracker](https://github.com/LLNL/krowkee/issues) to check for threads 
related to your desired contribution. 
We recommend creating an issue prior to issuing a pull request if you are 
planning significant code changes or have questions. 

# Contribution Workflow

These guidelines assume that the reader is familiar with the basics of 
collaborative development using git and GitHub.
This section will walk through our preferred pull request workflow for 
contributing code to `krowkee`. 
The tl;dr guidance is:
- Fork the [LLNL krowkee repository](https://github.com/LLNL/krowkee)
- Create a descriptively named branch 
  (`feature/myfeature`, `iss/##`, `hotfix/bugname`, etc) in your fork off of 
  the `develop` branch
- Commit code, following our [guidelines](#formatting-guidelines)
- Create a [pull request](https://github.com/LLNL/krowkee/compare) from your
  branch targeting the LLNL `develop` branch

## Forking krowkee

If you are not a `krowkee` developer at LLNL, you will not have permissions to 
push new branches to the repository.
Even `krowkee` developers at LLNL will want to use forks for most contributions.
This will create a clean copy of the repository that you own, and will allow for 
exploration and experimentation without muddying the history of the central 
repository.

If you intend to maintain a persistent fork of `krowkee`, it is a best practice 
to set the LLNL repository as the `upstream` remote in your fork. 
```
$ git clone git@github.com:your_name/krowkee.git
$ cd krowkee
$ git remote add upstream git@github.com:LLNL/krowkee.git
```
This will allow you to incorporate changes to the `main` and `develop` 
branches as they evolve.
For example, to your fork's develop branch perform the following commands:
```
$ git fetch upstream
$ git checkout develop
$ git pull upstream develop
$ git push origin develop
```
It is important to keep your develop branch up-to-date to reduce merge conflicts
resulting from future PRs.

## Contribution Types

Most contributions will fit into one of the follow categories, which by 
convention should be committed to branches with descriptive names.
Here are some examples:
- A new feature (`feature/<feature-name>`)
- A bug or hotfix (`hotfix/<bug-name>` or `hotfix/<issue-number>`)
- A response to a [tracked issue](https://github.com/LLNL/krowkee/issues) 
  (`iss/<issue-number>`)
- A work in progress, not to be merged for some time (`wip/<change-name>`)

### Developing a new feature

New features should be based on the develop branch:
```
$ git checkout develop
$ git pull upstream develop
```
You can then create new local and remote branches on which to develop your 
feature.
```
$ git checkout -b feature/<feature-name>
$ git push --set-upstream origin feature/<feature-name>
```
Commit code changes to this branch, and add tests to `${krowkee_ROOT}/tests` 
that validate the correctness of your code, modifying existing tests if need be.
Be sure to add `add_seq_krowkee_test(<test-name>)` or  
`add_mpi_krowkee_test(<test-name>)` to `tests/CMakeLists.txt`
as appropriate.
Be sure that you can build and that `make test` passes, and that your test
runs successfully.

Make sure that you follow our [formatting guidelines](#formatting-guidlines) for 
any changes to the source code or build system. 
If you create new methods or classes, please add Doxygen documentation 
(guidelines forthcoming). 

Once your feature is complete and your tests are passing, ensure that your 
remote fork is up-to-date and 
[create a PR](https://github.com/LLNL/krowkee/compare). 

### Developing a hotfix

Firstly, please check to ensure that the bug you have found has not already been
fixed in `develop`. 
If it has, we suggest that you either temporarily swap to the `develop` branch.

If you have identified an unsolved bug, you can document the problem and create
an [issue](https://github.com/LLNL/krowkee/issues).
If you would like to solve the bug yourself, follow a similar protocol to 
feature development.
First, ensure that your fork's `develop` branch is up-to-date.
```
$ git checkout develop
$ git pull upstream develop
```
You can then create new local and remote branches on which to write your bug 
fix.
```
$ git checkout -b hotfix/<bug-name>
$ git push --set-upstream origin hotfix/<bug-name>
```

Firstly, create a test added to `tests/` that reproduces the bug or 
modify an existing test to catch the bug if that is more appropriate.
Be sure to add `add_seq_krowkee_test(<test-name>)` or
`add_mpi_crqouis_test(<test-name>)` to `${YGM_ROOT}/tests/CMakeLists.txt`
if you create a new test case.
Then, modify the code to fix the bug and ensure that your new or modified test
case(s) pass via `make test`. 

Please update function and class documentation to reflect any changes as 
appropriate, and follow our [formatting guidlines](#formatting-guidelines) with 
any new code.

Once your are satisfied that the bug is fixed, ensure that your remote fork is 
up-to-date and [create a PR](https://github.com/LLNL/krowkee/compare). 

# Tests

`krowkee` uses GitHub actions for continuous integration tests. 
Our tests run automatically against every new commit and pull request, and pull
requests must pass all tests prior to being considered for merging into the main
project.
If you are developing a new feature or fixing a bug, please add a test or modify
existing tests that will ensure the correctness of the new code. 

`krowkee`'s tests are contained in the `test` directory, and new tests must be added
manually to `tests/CMakeLists.txt` by adding `add_seq_krowkee_test(<test-name>)`
in or `add_mpi_krowkee_test(<test-name>)` 
order to ensure that they are automatically built and checked via `make test`.

# Formatting Guidelines

## Naming Style

In general, all names in `krowkee` should be as succinct as possible while 
remaining descriptive.

`krowkee` function and variable names should all be `snake_case`, i.e. one or 
more lowercase words separated by underscores.
Mathy variable names like `x` and `y` should be avoided in favor of descriptive
names such as `range_size`.
All private variables and functions within classes and structs should be 
feature a single underscore prefix.

`krowkee` makes liberal use of local type aliases via `using` and `typedef` 
throughout.
All such local type aliases, i.e. those whose scope is limited to within a 
class, struct, or function, should also be `snake_case`, with the suffix `_t` 
applied to the name. 

Class and struct names and template parameter names should be `PascalCase`, i.e. 
one or more words starting with uppercase letters with no separator.

## Formatting Style

`krowkee` uses 
[clang-format](https://www.kernel.org/doc/html/v4.17/process/clang-format.html)
to guarantee a consistent format for C++ code.
Our style settings are located in 
[.clang-format](https://github.com/LLNL/krowkee/blob/main/.clang-format) in the 
project root.
`clang-format` is easy to use, and can be easily instrumented to auto-format
code using most modern editors:
- [vscode](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format)
- [sublime](https://packagecontrol.io/packages/Clang%20Format)
- [vi](https://github.com/rhysd/vim-clang-format)
- [emacs](https://github.com/sonatard/clang-format)

`krowkee` also uses [cmake-format](https://github.com/cheshirekow/cmake_format) to 
to guarantee a consistent format for cmake build code.
Our style settings are located in 
[.cmake-format.py](https://github.com/LLNL/krowkee/blob/main/.cmake-format.py) 
in the project root.


