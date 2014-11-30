default : all;

run : test;

test : all
	make -C kernel kernel.img
	(cd fat439; make clean; make)
	qemu-system-x86_64 -s -nographic --serial mon:stdio -hdc kernel/kernel.img -hdd fat439/user.img
	
% :
	(make -C kernel $@)
	(make -C user $@)
	(make -C fat439 $@)

