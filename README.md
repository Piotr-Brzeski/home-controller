# Home Controller
Smart home controller.

## Build
Use [XcodeGen](https://github.com/yonaskolb/XcodeGen) to generate Xcode project
or [NinjaGen](https://github.com/Piotr-Brzeski/NinjaGen) to generate ninja build file.

## Configuration

Example of configuration file:
```
{
	"tradfri":
	{
		"address": "127.0.0.1",
		"identity": "name",
		"key": "klucz"
	},
	"link":
	{
		"port": 1234
	},
	"devices":
	[
		{
			"name": "LampSwitch",
			"type": "button",
			"number": 1
		}
	],
	"operations":
	[
		{
			"name": "Toggle Office Lamp",
			"type": "toggle",
			"device": "Office Lamp 4"
		}
	],
	"commands":
	[
		{
			"device": "LampSwitch",
			"state": "click",
			"operation": "Toggle Office Lamp"
		}
	]
}
```
