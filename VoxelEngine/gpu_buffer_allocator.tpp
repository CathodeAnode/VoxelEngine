template<typename T>
GPUBufferAllocator<T>::GPUBufferAllocator(GLenum bufferType, GLenum bufferUsage, const std::vector<T>& data)
	: bufferSize(data.size()), type(bufferType), usage(bufferUsage)
{
	glGenBuffers(1, &bufferID);
	glBindBuffer(bufferType, bufferID);
	glBufferData(bufferType, bufferSize * sizeof(T), data.data(), bufferUsage);
	glBindBuffer(bufferType, 0);

	currentSize = bufferSize;
}

template<typename T>
GPUBufferAllocator<T>::GPUBufferAllocator(GLenum bufferType, GLenum bufferUsage, unsigned int size)
	: bufferSize(size), type(bufferType), usage(bufferUsage)
{
	if (bufferUsage == GL_STATIC_DRAW || bufferUsage == GL_STATIC_COPY || bufferUsage == GL_STATIC_READ) {
		throw std::logic_error("Static buffers must be initialized with data");
	}

	glGenBuffers(1, &bufferID);
	glBindBuffer(bufferType, bufferID);
	glBufferData(bufferType, bufferSize * sizeof(T), nullptr, bufferUsage);
	glBindBuffer(bufferType, 0);

	currentSize = 0;
}

template<typename T>
GPUBufferAllocator<T>::~GPUBufferAllocator()
{
	glDeleteBuffers(1, &bufferID);
}

template<typename T>
void GPUBufferAllocator<T>::upload(const std::vector<T>& data)
{
	if (usage == GL_STATIC_DRAW || usage == GL_STATIC_COPY || usage == GL_STATIC_READ) {
		throw std::logic_error("Static buffers cannot be modifiy");
	}

	if (data.size() > bufferSize) {
		resize(data.size());
	}

	glBindBuffer(type, bufferID);
	glBufferSubData(type, 0, data.size() * sizeof(T), data.data());
	glBindBuffer(type, 0);

	currentSize = data.size();
}

template<typename T>
void GPUBufferAllocator<T>::append(T data)
{
	if (currentSize + 1 > bufferSize) {
		resize(bufferSize * 2 + 1); // grow strategy
	}
	glBindBuffer(type, bufferID);
	glBufferSubData(type, currentSize * sizeof(T), sizeof(T), &data);
	glBindBuffer(type, 0);
	
	currentSize++;
}

template<typename T>
void GPUBufferAllocator<T>::append(const std::vector<T>& data)
{
	if (currentSize + data.size() > bufferSize) {
		resize(bufferSize * 2  + 1);
	}

	glBindBuffer(type, bufferID);
	glBufferSubData(type, currentSize * sizeof(T), data.size() * sizeof(T), data.data());
	glBindBuffer(type, 0);

	currentSize += data.size();
}

template<typename T>
void GPUBufferAllocator<T>::insert(T data, unsigned int index)
{
	if (currentSize + 1 > bufferSize) {
		resize(bufferSize * 2 + 1); // grow strategy
	}

	move(index, index + 1, currentSize - index);

	glBindBuffer(type, bufferID);
	glBufferSubData(type, index * sizeof(T), sizeof(T), &data);
	glBindBuffer(type, 0);

	currentSize++;
}

template<typename T>
void GPUBufferAllocator<T>::insert(const std::vector<T>& data, unsigned int index)
{
	if (currentSize + data.size() > bufferSize) {
		resize(bufferSize * 2 + 1);
	}

	if (index >= bufferSize) {
		throw std::logic_error("Index out of range.");
	}

	move(index, index + data.size(), currentSize - index);

	glBindBuffer(type, bufferID);
	glBufferSubData(type, index * sizeof(T), sizeof(T) * data.size(), data.data());
	glBindBuffer(type, 0);

	currentSize += data.size();
}

template<typename T>
void GPUBufferAllocator<T>::replace(const std::vector<T>& data, unsigned int index, unsigned int oldSize)
{
	if (currentSize + (data.size() - oldSize) > bufferSize) {
		resize(bufferSize * 2 + 1);
	}

	if (index >= bufferSize) {
		throw std::logic_error("Index out of range.");
	}

	move(index + oldSize, index + data.size(), currentSize - index + oldSize);

	glBindBuffer(type, bufferID);
	glBufferSubData(type, index * sizeof(T), sizeof(T) * data.size(), data.data());
	glBindBuffer(type, 0);

	currentSize += (data.size() - oldSize);

}

template<typename T>
void GPUBufferAllocator<T>::resize(unsigned int newSize)
{
	if (newSize == bufferSize)
		return;

	// create temp buffer to copy current data into
	unsigned int copyBuffer;
	glGenBuffers(1, &copyBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
	glBufferData(GL_COPY_WRITE_BUFFER, currentSize * sizeof(T), nullptr, GL_STATIC_COPY);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	// copy data into temp copy buffer
	glBindBuffer(GL_COPY_READ_BUFFER, bufferID);
	glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, currentSize * sizeof(T));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	// expand main buffer to new size
	glDeleteBuffers(1, &bufferID);
	glGenBuffers(1, &bufferID);
	glBindBuffer(type, bufferID);
	glBufferData(type, newSize * sizeof(T), nullptr, usage);
	glBindBuffer(type, 0);

	// copy old data into newly resized buffer
	glBindBuffer(GL_COPY_READ_BUFFER, copyBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, bufferID);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, currentSize * sizeof(T));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	// delete temp copy buffer & set new size on object
	glDeleteBuffers(1, &copyBuffer);
	bufferSize = newSize;

}

template<typename T>
void GPUBufferAllocator<T>::move(unsigned int startIndex, unsigned int endIndex, unsigned int size)
{
	// if intervial ranges overlap
	if (std::max(startIndex, endIndex) <= std::min(startIndex + size , endIndex + size)) {

		// create temp buffer to copy current data into
		unsigned int copyBuffer;
		glGenBuffers(1, &copyBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
		glBufferData(GL_COPY_WRITE_BUFFER, size * sizeof(T), nullptr, GL_STATIC_COPY);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// copy data into temp copy buffer
		glBindBuffer(GL_COPY_READ_BUFFER, bufferID);
		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, startIndex * sizeof(T), 0, size * sizeof(T));
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// copy from temp buffer to move location
		glBindBuffer(GL_COPY_READ_BUFFER, copyBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, bufferID);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, endIndex * sizeof(T), size * sizeof(T));
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// delete temp copy buffer
		glDeleteBuffers(1, &copyBuffer);
	}
	else {
		glBindBuffer(GL_COPY_READ_BUFFER, bufferID);
		glBindBuffer(GL_COPY_WRITE_BUFFER, bufferID);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, startIndex * sizeof(T), endIndex * sizeof(T), sizeof(T) * size);
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	}
}
