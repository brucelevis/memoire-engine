#pragma once
#include <string>
class Transform;

class Debug
{
public:
	static void Log(std::string value);
	static void DrawTransform(Transform* t);
private:

};