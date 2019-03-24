Using GitHub
============

GitHub contributions typically work by creating a fork of the project on your user account, making a branch on that fork to work on and then filing a pull request to upstream your code.  This is how you would go about it.

Forking
-------

Go to the `libMariaS3 GitHub page <https://github.com/mariadb-corporation/libmarias3>`_ and click the *Fork* button near the top.  Once you have forked you can get a local copy of this fork to work on (where *user* is your username):

.. code-block:: bash

   git clone https://github.com/user/libmarias3.git

You then need to make your local clone aware of the upstream repository:

.. code-block:: bash

   cd libmarias3
   git remote add upstream https://github.com/mariadb-corporation/libmarias3.git

Branch
------

Before creating a branch to work on you should first make sure your local copy is up to date:

.. code-block:: bash

   git checkout master
   git pull --ff-only upstream master
   git push

You can then create a branch from master to work on:

.. code-block:: bash

   git checkout -b a_new_feature

Hack on code!
-------------

Hack away at your feature or bug.

Test
----

Once your code is ready the test suite should be run locally:

.. code-block:: bash

   make
   make check

If there are documentation changes you should install ``python-sphinx`` and try to build the HTML version to run a syntax check:

.. code-block:: bash

   make html

Commit and push
---------------

If you have never contributed to GitHub before then you need to setup git so that it knows you for the commit message:

.. code-block:: bash

   git config --global user.name "Real Name"
   git config --global user.email "me@me.com"

Make sure you use `git add` to add any new files to the repository and then commit:

.. code-block:: bash

   git commit -a

Your editor will pop up to enter a commit messages above the comments.  The first line should be no more than 50 characters and be a subject of the commit.  The second line should be blank.  The third line onwards can contain details and these should be no more than 72 characters each.

If your commit fixes an issue you can add the following (for issue #93)::

   Fixes mariadb-corporation/libmarias3#93

Once all your commits are done a quick rebase may be needed to make sure your changes will merge OK with what is in master:

.. code-block:: bash

   git fetch upstream
   git rebase -i upstream/master

This should bring up a commit-style message in the editor with *pick* as the first word.  Save this and the rebase will complete.  If the rebase tells you there is a conflict you will need to locate the problem using ``git diff``, fix it and do:

.. code-block:: bash

   git add <filename>
   git rebase --continue

If things look like they are going wrong you can undo the rebase using the following and can get in touch with us:

.. code-block:: bash

   git rebase --abort

You should now be ready to push up to GitHub:

.. code-block:: bash

   git push --set-upstream origin a_new_feature

If you go to your repository on GitHub's website you will an option to file a *Pull Request*.  Use this to submit a pull request upstream for your branch.

Help
----

If you get stuck at any point feel free to reach out to us by filing an issue on Github.
