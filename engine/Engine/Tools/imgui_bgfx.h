#pragma once

namespace me {
	namespace imgui {
		void create();
		void beginFrame();
		void endFrame();
		void destroy();

		struct data {
			static float scroll;
		};
	}
}