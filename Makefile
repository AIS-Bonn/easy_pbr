all:
	echo "Building EasyPBR"
	python3 -m pip install -v --user --editable ./

clean:
	python3 -m pip uninstall easypbr
	rm -rf build *.egg-info build easypbr*.so libeasypbr_cpp.so libeasygl_cpp.so
