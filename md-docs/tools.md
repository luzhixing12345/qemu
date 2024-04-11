
# tools

## VMware

VMware可以说是虚拟化技术的布道者,这家成立于1998年的公司虽然涉足时间很短,但仅用一年时间就发布了重量级产品workStations1.0,扰动了沉寂多年的虚拟化市场.2001年,又通过发布GSX和ESX一举奠定了行业霸主的地位.如此快速的成长无疑也是站在了巨人【Linux】的肩膀上.而对于VMware在其hypervisor ESXi中非法使用Linux内核源代码的指控一刻也没有停止过.

## Xen

VMware的成功引发了业界极大的恐慌.IBM、AMD、HP、Red Hat和Novell等厂商纷纷加大了对虚拟化技术的投资,选择的投资对象是由英国剑桥大学与1990年便发起的一个虚拟化开源项目Xen.相比VMware,Xen选择采用半虚拟化技术提升虚拟化的性能.商业公司的投入很快催熟了Xen.2003年Xen1.0问世.Xen的推出使虚拟化领域终于出现了能与VMware竞争的对手.Linux厂商Red Hat、Novell等公司纷纷在自己的操作系统中包含了各自版本的Xen.Xen的创始人为了基于Xen hypervisor能够提供更完善的虚拟化解决方案,更好地与其它虚拟化产品(如VMware ESX)竞争,也成立了他们自己的公司

## KVM

Xen的出现顺应了IT大佬们抢占市场的潮流,但由于Xen与Linux采用不改造Linux内核而是采用补丁的松耦合方式,因此需要在Linux的各种版本上打众多补丁.而Linux本身又处于飞速发展事情,版本日新月异.这使得Xen使用起来非常不便.这也为KVM的出现埋下了伏笔.

2006年10月,以色列的一家小公司Qumranet开发了一种新的"虚拟化"实现方案_即通过直接修改Linux内核实现虚拟化功能(Kernel-Based Virtual Machine).这种与Linux融为一体的方式很快进入了Linux厂商的视线.很快于2007年KVM顺利合入了Linux2.6.20主线版本.而作为Linux领域老大的redhat,一方面对在Linux内核中直接发展虚拟化有着浓厚的兴趣,另一方面也不甘于被Citrix所引导的Xen牵着鼻子走,最终于2008年,以一亿七百万的价格收购了Qumranet,并将自己的虚拟化阵营由Xen切换为KVM.

## Hyper-V

最后我们再来说说操作系统领域的霸主Microsoft.Linux的崛起已经让这位霸主感受到了前所未有的挑战,而00年后虚拟化技术进入爆发期,诸多厂商如雨后春笋般涌现,更让这位霸主有些应接不暇.凭借庞大的体量,Microsoft也开始频频出招.2003年收购Connectix获得虚拟化技术并很快推出Virtual Server;2007年与Citrix签署合作协议,并在2008年年底推出服务器虚拟化平台Hyper-V.至此我们可以看出由于是与Citrix深度合作,因此Hyper-V的架构与Xen类似,也属于半虚拟化技术.

## 参考

- [Hypervisor(VMM)基本概念及分类](https://bbs.huaweicloud.com/blogs/369191)