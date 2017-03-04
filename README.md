# git-junction

Helps to administer an assorted set of git repositories in a shared location.

Users of git often find themselves in need of a location where to set up public
repositories. There are a number of alternatives, some of which are commercial
web sites, like [github](http://github.com). These are suitable, among others,
for open source projects in which participants don't have a pre-existing
relationship with each other.

However, these commercial web sites aren't suitable for every project, and they
don't cover all use cases of what git repositories can be used for.

Setting up *git-junction* is one alternative for those simple and
straightforward cases where you need to let your colleagues access your
repository. It has been designed for private projects, where people don't need
the extra communication tools (like pull requests, issues or wikis), where
people have servers of their own and where it's not necessarily possible to send
the source code abroad.

## components

*git-junction* implements only a part of the whole, and works with standard
Linux services and other open source applications for the rest:

- [*cgit*](https://git.zx2c4.com/cgit/) - for accessing repositories over WWW
- [*git-daemon*](https://git-scm.com/) - for easy and fast access to public git
  repositories
- [*ssh*](http://www.openssh.com/) - for accessing your private git repositories
  and using git-junction console
- [*systemd*](https://www.freedesktop.org/wiki/Software/systemd/) - for running
  daemons and scheduled tasks

## details

- *git-junction* implements its own user management. It uses two system
  accounts: 1. `git-console` for setting up repositories and SSH-keys, and 2.
  `git` for accessing private repositories.
- Two use cases have been implemented: 1. mirroring an external public
  repository, and 2. sharing your own local repository.
- All repositories have an owner. Only the owner can configure the repository.
- Sharing your repository can be done in two ways: 1. "no-auth" read/write
  access for everyone, and 2. public read access with private write access for
  the owner.
- The abstraction level is intentionally higher than with your normal git
  commands. If you need lower abstraction level, you can set up such a system
  with `git-shell`.
- Many helpful features are still unimplemented, like renaming repositories or
  changing repository owners.
