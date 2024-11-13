sudo apt install android-tools-fsutils

build custom.img cmd:
make_ext4fs -s -l 100M -a custom custom.img custom



edge2-io-spi.dts:
/dts-v1/;
/plugin/;

/ {
	fragment@0 {
		target = <&spi1>;

		__overlay__ {
			status = "disabled";
		};
	};
};

dtc -I dts -O dtb -o edge2-io-spi.dtbo edge2-io-spi.dts