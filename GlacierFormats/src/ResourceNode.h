#pragma once
#include <variant>
#include <memory>
#include "PRIM.h"
#include "BORG.h"
#include "RPKG.h"
#include "TEXD.h"
#include "MATI.h"
#include "TEXT.h"
#include "MATE.h"

namespace GlacierFormats {

	using EMPTY = void*;
	using ResourceVariant = std::variant<EMPTY, PRIM*, BORG*, TEXT*, TEXD*, MATI*, MATE*>;

	template<typename T>
	class ResourceNode;

	class IResourceNode {
	private:
		ResourceVariant type_tag;

	protected:
		template<typename T>
		IResourceNode(T*) : type_tag((T*) nullptr) {};

	public:
		template<typename T>
		bool isType() const;

		template<typename T>
		const ResourceNode<T>* get_if() const;

		template<typename T>
		const ResourceNode<T>* get() const;

		virtual RuntimeId id() = 0;
		virtual void insertChildNode(std::unique_ptr<IResourceNode> node) = 0;
		virtual std::vector<std::unique_ptr<IResourceNode>>& children() = 0;
		virtual const std::vector<std::unique_ptr<IResourceNode>>& children() const = 0;
	};

	template<typename T>
	inline bool IResourceNode::isType() const {
		return std::holds_alternative<T*>(type_tag);
	}

	template<typename T>
	inline const ResourceNode<T>* IResourceNode::get_if() const
	{
		if (isType<T>())
			return static_cast<const ResourceNode<T>*>(this);
		return nullptr;
	}

	template<typename T>
	inline const ResourceNode<T>* IResourceNode::get() const {
		return static_cast<const ResourceNode<T>*>(this);
	}

	template<typename T>
	class ResourceNode : public IResourceNode {
	private:
		RuntimeId id_;
		std::unique_ptr<T> resource_;
		std::vector<std::unique_ptr<IResourceNode>> children_;

	public:
		ResourceNode(RuntimeId id, std::unique_ptr<T> resource) :
			IResourceNode((T*)nullptr),
			id_(id),
			resource_(std::move(resource)) {

		}

		const std::unique_ptr<T>& resource() const;

		RuntimeId id();
		std::vector<std::unique_ptr<IResourceNode>>& children();
		const std::vector<std::unique_ptr<IResourceNode>>& children() const;

		void insertChildNode(std::unique_ptr<IResourceNode> node);
	};

	template<typename T>
	inline const std::unique_ptr<T>& ResourceNode<T>::resource() const {
		return resource_;
	}

	template<typename T>
	inline RuntimeId ResourceNode<T>::id()
	{
		return id_;
	}

	template<typename T>
	inline std::vector<std::unique_ptr<IResourceNode>>& ResourceNode<T>::children() {
		return children_;
	}

	template<typename T>
	inline const std::vector<std::unique_ptr<IResourceNode>>& ResourceNode<T>::children() const {
		return children_;
	}

	template<typename T>
	inline void ResourceNode<T>::insertChildNode(std::unique_ptr<IResourceNode> node) {
		children_.emplace_back(std::move(node));
	}

}