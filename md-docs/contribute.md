
# contribute

```bash
git format-patch -s --stdout -1 > patch
```

```bash
./scripts/get_maintainer.pl -f docs/system/devices/cxl.rst
```

```bash
git config diff.renames true
git config diff.algorithm patience
git config sendemail.cccmd 'scripts/get_maintainer.pl --nogit-fallback'
git config sendemail.to qemu-devel@nongnu.org
```

```bash
git send-email patch
```

> --to=qemu-devel@nongnu.org 

![20240804131057](https://raw.githubusercontent.com/learner-lu/picbed/master/20240804131057.png)

[qemu-devel Archives](https://lists.nongnu.org/archive/html/qemu-devel/)

[patchew QEMU](https://patchew.org/QEMU/)

[tinylab mailing-list-intro](https://tinylab.org/mailing-list-intro/)

[nongnu lists](https://lists.nongnu.org/archive/html/qemu-devel/2024-07/msg05067.html)