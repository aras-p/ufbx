#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if !defined(UFBX_NO_MALLOC)

static uint64_t ufbxi_win32_total_memory = 0;

volatile LONG ufbxi_win32_lock_init;
CRITICAL_SECTION ufbxi_win32_lock;

void ufbx_malloc_os_lock()
{
	LONG prev = InterlockedOr(&ufbxi_win32_lock_init, 1);
	if (prev == 0) {
		InitializeCriticalSectionAndSpinCount(&ufbxi_win32_lock, 100);
		InterlockedOr(&ufbxi_win32_lock_init, 2);
		prev = 3;
	}
	while (prev < 2) {
		prev = InterlockedOr(&ufbxi_win32_lock_init, 0);
	}

	EnterCriticalSection(&ufbxi_win32_lock);
}

void ufbx_malloc_os_unlock()
{
	LeaveCriticalSection(&ufbxi_win32_lock);
}

void *ufbx_malloc_os_allocate(size_t size, size_t *p_allocated_size)
{
	size_t min_size = 4*1024*1024;
	size_t alloc_size = size > min_size ? size : min_size;
	void *pointer = VirtualAlloc(NULL, alloc_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pointer) return NULL;

	ufbxi_win32_total_memory += alloc_size;

	*p_allocated_size = alloc_size;
	return pointer;
}

bool ufbx_malloc_os_free(void *pointer, size_t allocated_size)
{
	if (ufbxi_win32_total_memory >= 32*1024*1024) {
		ufbxi_win32_total_memory -= allocated_size;
		VirtualFree(pointer, 0, MEM_RELEASE);
		return TRUE;
	} else {
		return FALSE;
	}
}

#endif

#define UFBXI_WIN32_MAX_FILENAME_WCHAR 1024

void *ufbx_stdio_open(const char *path, size_t path_len)
{
	WCHAR filename_wide[UFBXI_WIN32_MAX_FILENAME_WCHAR + 1] = { 0 };
	int len = MultiByteToWideChar(CP_UTF8, 0, path, path_len, filename_wide, UFBXI_WIN32_MAX_FILENAME_WCHAR);
	if (len < 0 || len >= UFBXI_WIN32_MAX_FILENAME_WCHAR) return NULL;
	filename_wide[len] = 0;

	HANDLE file = CreateFileW(filename_wide, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == NULL || file == INVALID_HANDLE_VALUE) return NULL;

	return file;
}

size_t ufbx_stdio_read(void *file, void *data, size_t size)
{
	HANDLE h = (HANDLE)file;
	size_t offset = 0;
	while (offset < size) {
		size_t to_read = size - offset;
		if (to_read >= 0x20000000) to_read = 0x20000000;
		DWORD num_read = 0;
		BOOL ok = ReadFile(h, (char*)data + offset, (DWORD)to_read, &num_read, NULL);
		if (!ok) return SIZE_MAX;
		if (num_read == 0) break;
		offset += num_read;
	}
	return offset;
}

bool ufbx_stdio_skip(void *file, size_t size)
{
	HANDLE h = (HANDLE)file;
	LARGE_INTEGER move_amount;
	move_amount.QuadPart = size;
	return SetFilePointerEx(h, move_amount, NULL, FILE_CURRENT);
}

uint64_t ufbx_stdio_size(void *file)
{
	LARGE_INTEGER size;
	if (!GetFileSizeEx(file, &size)) return 0;
	return size.QuadPart;
}

void ufbx_stdio_close(void *file)
{
	HANDLE h = (HANDLE)file;
	CloseHandle(h);
}

#endif
