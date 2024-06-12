#include <konide.h>
#include <konide/generic/KonideSceneLayer.h>
#include <SDL.h>
#include <SDL_Vulkan.h>

#pragma comment(lib, "konide.lib")

extern int demo_main();

#ifdef _WIN32

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{
	return demo_main();
}
#else
int main()
{
	return demo_main();
}
#endif

int demo_main()
{
	SDL_Window* window;
	SDL_Surface* surface;
	SDL_Event event;

	bool bShouldQuit = false;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		return 1;
	}

	int width = 1280, height = 720;

	window = SDL_CreateWindow("Hello Triangle!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

	surface = SDL_GetWindowSurface(window);

	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xFF, 0x00, 0xFF));

	SDL_UpdateWindowSurface(window);

	// Konide example starts
	KonideRenderer Renderer(KONIDE_RENDER_FEATURE_SWAPCHAIN);

	unsigned int pExtCount = 0;

	SDL_Vulkan_GetInstanceExtensions(window, &pExtCount, 0);

	std::vector<const char*> SDLExtensions(pExtCount);

	if (SDL_Vulkan_GetInstanceExtensions(window, &pExtCount, SDLExtensions.data()) == SDL_FALSE)
	{
		return 2;
	}

	Renderer.Initialize(SDLExtensions);

	VkInstance instance = Renderer.GetInstance();
	VkSurfaceKHR vksurface;

	if(SDL_Vulkan_CreateSurface(window, instance, &vksurface) == SDL_FALSE)
	{
		return 3;
	}

	Renderer.SetSurface(vksurface);
	
	Renderer.CreateDevice();

	Renderer.CreateSwapchain(width, height);

	Renderer.AddLayer<KonideSceneLayer>();

	// Konide example ends

	while (!bShouldQuit)
	{
		SDL_PollEvent(&event);

		if (event.type == SDL_QUIT)
		{
			bShouldQuit = true;
		}

		Renderer.FlushRender();
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}