#pragma once
#include <map>
#include <string>

#include "Engine/Nodes/Node.h"
#include "DebugMemory.h"

namespace me {
	namespace util {
		template<typename T>
		Node* createInstance() {
			return DBG_NEW T;
		}

		typedef std::map<std::string, Node* (*)()> nodeMapType;

		class ObjectFactory
		{
		public:
			static void init();
			static void destroy();
			static Node* createObject(std::string const& s);

		private:
			static nodeMapType nodeMap;
		};
	}
}

