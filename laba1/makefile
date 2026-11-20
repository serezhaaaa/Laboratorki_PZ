all: raster

raster: main.o Raster.o Shapes.o
	g++ main.o Raster.o Shapes.o -o raster

main.o: main.cpp include/Raster.h include/Shapes.h
	g++ -std=c++17 -Iinclude -c main.cpp

Raster.o: src/Raster.cpp include/Raster.h
	g++ -std=c++17 -Iinclude -c src/Raster.cpp

Shapes.o: src/Shapes.cpp include/Raster.h include/Shapes.h
	g++ -std=c++17 -Iinclude -c src/Shapes.cpp

samples: raster
	./raster --samples images

clean:
	rm -f *.o raster
