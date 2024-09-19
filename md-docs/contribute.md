
# contribute

```bash
git format-patch -s --subject-prefix='PATCH' -1
```

如果是 v2 版本可以使用

```bash
git format-patch -s --subject-prefix='PATCH' -1 -v2
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

[kernel lore](https://lore.kernel.org/qemu-devel/) send-email

## 参考

- [开源patch提交备忘](https://blog.csdn.net/huang987246510/article/details/118343051)
- [ouonline how-to-submit-patches-to-linux-kernel](https://ouonline.net/how-to-submit-patches-to-linux-kernel)