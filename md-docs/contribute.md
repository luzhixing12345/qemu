
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
```

```bash
git send-email --to=qemu-devel@nongnu.org patch
```

[qemu-devel Archives](https://lists.nongnu.org/archive/html/qemu-devel/)

[patchew QEMU](https://patchew.org/QEMU/)