all: demo sine cosine

demo:
	gcc -D DEMO -o tri_demo tri_approx.c

sine:
	gcc -D SINE -o tri_sine tri_approx.c

cosine:
	gcc -D COSINE -o tri_cosine tri_approx.c

clean:
	rm -rf tri_demo tri_sine tri_cosine
