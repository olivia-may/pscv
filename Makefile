.PHONY: pscv clean

pscv:
	rm -f pscv
	gcc pscv.c -o pscv

clean:
	rm pscv
