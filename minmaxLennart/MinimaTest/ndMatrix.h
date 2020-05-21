#pragma once

#include <vector>
#include <assert.h>

template<typename T>
class NDMatrix {
	std::vector<int> dimentions;

public:
	T* data;

	size_t getTotalSize() const {
		size_t totalSize = 1;
		for(int dimentionSize : this->dimentions) {
			totalSize *= dimentionSize;
		}
		return totalSize;
	}

	NDMatrix(const std::vector<int>& dimentions) : dimentions(dimentions) {
		data = new T[getTotalSize()];
	}

	~NDMatrix() {
		delete[] data;
	}

	NDMatrix(const NDMatrix&) = delete;
	NDMatrix& operator=(const NDMatrix&) = delete;
	NDMatrix(const NDMatrix&&) = delete;
	NDMatrix& operator=(const NDMatrix&&) = delete;

	size_t getIndexFor(const std::vector<int>& indices) const {
		size_t totalIndex = 0;
		int multiplier = 1;

		for(int i = 0; i < indices.size(); i++) {
			assert(indices[i] >= 0 && indices[i] < this->dimentions[i]);
			totalIndex += indices[i] * multiplier;
			multiplier *= this->dimentions[i];
		}

		return totalIndex;
	}

	const std::vector<int>& getDimentions() const {
		return this->dimentions;
	}

	T& operator[](const std::vector<int>& indices) {
		assert(indices.size() == this->dimentions.size());
		return data[getIndexFor(indices)];
	}

	const T& operator[](const std::vector<int>& indices) const {
		assert(indices.size() == this->dimentions.size());
		return data[getIndexFor(indices)];
	}

	T getOrDefault(const std::vector<int>& indices, const T& def) {
		assert(indices.size() == this->dimentions.size());

		size_t totalIndex = 0;
		int multiplier = 1;

		for(int i = 0; i < indices.size(); i++) {
			if(indices[i] < 0 || indices[i] >= this->dimentions[i]) {
				return def;
			}
			totalIndex += indices[i] * multiplier;
			multiplier *= this->dimentions[i];
		}
		return data[totalIndex];
	}
};
