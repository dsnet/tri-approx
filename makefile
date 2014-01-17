all: demo sine cosine stats

demo:
	gcc -D DEMO -o tri_demo tri_approx.c -lm

sine:
	gcc -D SINE -o tri_sine tri_approx.c -lm

cosine:
	gcc -D COSINE -o tri_cosine tri_approx.c -lm

stats:
	gcc -D STATS -o tri_stats tri_approx.c -lm

clean:
	rm -rf tri_demo tri_sine tri_cosine tri_stats
