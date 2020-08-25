#include "keyfile.h"
#include "mem.h"

KeyFile* OpenKeyFile(const char * filename) {

	FILE* file = fopen(filename, "rb");

	if (!file) {
		return NULL;
	}

	KeyFile* kf = (KeyFile*)gff_calloc(1, sizeof(KeyFile));
	if (!kf) {
		fclose(file);
		return NULL;
	}

	size_t read = fread(kf, sizeof(KeyHeader), 1, file);

	if (read != 1) {

		gff_free(kf);
		fclose(file);
		return NULL;
	}

	if (memcmp(kf->Header.FileType, "KEY ", 4) != 0 || 
		memcmp(kf->Header.FileVersion, "V1  ", 4) != 0) {

		gff_free(kf);
		fclose(file);
		return NULL;
	}

	kf->file = file;

	return kf;
}

void CloseKeyFile(KeyFile* file) {

	if (file) {

		if (file->file) {
			fclose(file->file);
			file->file = NULL;
		}

		gff_free(file);
	}
}