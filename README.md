# SelfHackingApp
This repo contains an example of how to create self-modifying code in C++. This works by allowing code to rewrite itself using injected x86/x64 assembly instructions.

This code is 100% cross-platform, however this repo is currently only set up for Widows + Visual Studio. I wrote about this repo here: https://medium.com/squallygame/how-we-wrote-a-self-hacking-game-in-c-d8b9f97bfa99

Feel free to submit a pull request with a CMake solution that works on Linux/OSX/Windows.

## Example:
```cpp
NO_OPTIMIZE
int hackableRoutineTakeDamage(int health, int damage)
{
	// This is the code we want to be hackable by the user
	HACKABLE_CODE_BEGIN();
	health -= damage;
	HACKABLE_CODE_END();

	HACKABLES_STOP_SEARCH();

	if (health < 0)
	{
		health = 0;
	}

	return health;
}
END_NO_OPTIMIZE
```

Which can be modified via:
```cpp
auto funcPtr = &hackableRoutineTakeDamage;
std::vector<HackableCode*> hackables = HackableCode::create((void*&)funcPtr);

HackableCode* hackableCode = hackables[0];

// Disable taking damage by injecting a 'nop' over the original 'health -= damage;' code. Remaining bytes are filled with 'nops'
hackableCode->applyCustomCode("nop")
```
