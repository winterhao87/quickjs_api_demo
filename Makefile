
PREFIX=/usr/local

LIBQJS_INCLUDE_PATH=${PREFIX}/include
LIBQJS_LIB_PATH=${PREFIX}/lib/quickjs
LIBQJS_NAME=quickjs


api: api.c
	$(CC) -Wall -g -O2 -o $@ $< -l${LIBQJS_NAME} -I${LIBQJS_INCLUDE_PATH} -L${LIBQJS_LIB_PATH} -lm -ldl -lpthread 

.PHONY: clean

clean:
	rm -rf *.o api
