#include"Lapis.hpp"

#ifdef _WIN32
int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
) {
	lapis::lapisWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}
#endif

int main(int argc, char* argv[])
{
	lapis::lapisMain(argc, argv);
}