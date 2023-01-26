# source file and folder names
srcC=$(shell find ./src -name "*.c")
srcCuda=$(shell find ./src -name "*.cu")
src=$(srcC) $(srcCuda)
srcSubDirs=$(shell find ./src -type d)

# object file names
objectsC=$(srcC:./src/%.c=./obj/%.o)
objectsCuda=$(srcCuda:./src/%.cu=./obj/%.o)
objects=$(objectsC) $(objectsCuda)

# header file names
headers=$(shell find ./src -name "*.h")

# output directories
objectsDir=./obj
objectsSubDirs=$(srcSubDirs:./src/%=./obj/%)
srcDir=./src
binDir=./bin

# compiler
CC=nvcc
INCLUDES=-I./src
DEBUG=-g -G -DLOG_LEVEL=3
FAST=-O3 --use_fast_math -DLOG_LEVEL=1
CPP=-x c++ --compiler-options -fpermissive	# usare anche per compilare i file .c
CUDA=-x cu

# compila tutti i file .c e .h in src e sue sottocartelle in files .o in obj ottimizzando le informazioni di debug.
# In questa maniera sono compilati solo i sorgenti che sono stati modificati, velocizzando la build.
$(objectsC): $(objectsDir)/%.o: $(srcDir)/%.c $(headers)
	mkdir -p $(objectsDir) $(objectsSubDirs)
	$(CC) $(CPP) $(INCLUDES)  $(DEBUG) -c $< -o $@

$(objectsCuda): $(objectsDir)/%.o: $(srcDir)/%.cu $(headers)
	mkdir -p $(objectsDir) $(objectsSubDirs)
	$(CC) $(CUDA) $(INCLUDES) $(DEBUG) -dc $< -o $@

# linka i file .o in obj e crea il file debug
$(binDir)/debug: $(objectsC) $(objectsCuda)
	mkdir -p $(binDir)
	$(CC) $^ -o $@

# compila tutti i sorgenti in src in un eseguibile ottimizzato per le prestazioni
$(binDir)/release: $(src)
	mkdir -p $(binDir)
	$(CC) $(CPP) $(INCLUDES) $(FAST) -c $(srcC)
	$(CC) $(CUDA) $(INCLUDES) $(FAST) -dc $(srcCuda)
	$(CC) *.o -o $@
	rm *.o

all: debug

clean:
	rm -rf $(objectsDir) $(binDir)