#pragma once

namespace Game {
	struct SceneRootNode;

	class SceneComponent {
	public:
		virtual ~SceneComponent() {}

		virtual void update(float delta) = 0;
		virtual void initialize() = 0;
		virtual void finalize() = 0;
		virtual size_t  componentSize() = 0;

		SceneComponent(SceneRootNode* root) :root(root),active(false) {}

		bool IsActivated() { return active; }
	protected:
		SceneRootNode* root;
		bool active;
	};
}