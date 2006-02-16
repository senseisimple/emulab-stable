// Don't forget: heap is from heap[1] to heap[initSize]

public class MinHeap {
    private float[] heap;
    private int heapSize;

    public MinHeap(float[] a, int initSize) {
	if (initSize < a.length+1) {
	    throw new IllegalArgumentException("in constructor MinHeap, initSize must be >= to a.length+1");
	}
	heap = new float[initSize];
	for (int i = 0; i < a.length; ++i) {
	    heap[i+1] = a[i];
	}
	heapSize = a.length;
	buildMinHeap();
    }

    public MinHeap(int[] a) {
	heap = new float[a.length+1];
	for (int i = 0; i < a.length; ++i) {
	    heap[i+1] = a[i];
	}
	heapSize = a.length;
	buildMinHeap();
    }

    public void printHeapAsArray() {
	for (int i = 1; i <= heapSize; i++) {
	    System.out.print(heap[i]+" ");
	}
	System.out.println();
    }

    public void printHeapAsTree() {
	int depth = depth();
	int maxPrintableAtThisDepth = 1;
	int printedSoFar = 0;
	int spacing = 24;

	for (int i = 1; i <= heapSize; i++) {
	    if (printedSoFar == maxPrintableAtThisDepth) {
		maxPrintableAtThisDepth *= 2;
		printedSoFar = 0;
		System.out.print("\n");
		spacing /= 2;
	    }
	    for (int j = 0; j < spacing; j++) {
	        System.out.print(" ");
	    }
	    System.out.print(heap[i]);
	    for (int j = 0; j < spacing; j++) {
	        System.out.print(" ");
	    }
	    printedSoFar++;
	}
	System.out.println();
    }

    // Assume that integers are only max of 2 chars for testing
    public int depth() {
	int depth = 0;
	int i = 1;
	while (i < heapSize) {
	    depth += 1;
	    i = left(i);
	}
	return depth;
    }

    public void minHeapInsert(float key) {
	// check to see if we need to expand the array
	if (heap.length-1 == heapSize) {
	    float[] tmp = new float[heap.length];
	    System.arraycopy(heap,1,tmp,1,heapSize);
	    heap = new float[heapSize*2];
	    System.arraycopy(tmp,1,heap,1,heapSize);
	}

	heapSize++;
	heap[heapSize] = Integer.MIN_VALUE;
	heapDecreaseKey(heapSize,key);
    }

    public float heapExtractMin() {
	if (heapSize < 1) {
	    return -1;
	}
	float min = heap[1];
	heap[1] = heap[heapSize];
	heapSize--;
	minHeapify(1);
	return min;
    }

    public void heapDecreaseKey(int i,float key) {
	if (key > heap[i]) {
	    // add error statement;
	}
	heap[i] = key;
	while(i > 1 && heap[parent(i)] > heap[i]) {
	    float tmp = heap[parent(i)];
	    heap[parent(i)] = heap[i];
	    heap[i] = tmp;
	    i = parent(i);
        }
    }

    public float heapMinimum() {
	return heap[1];
    }

    private void buildMinHeap() {
	for (int i = heapSize/2; i > 0; --i) {
	    minHeapify(i);
	}
    }
    
    private void minHeapify(int i) {
	int left = left(i);
	int right = right(i);

	int smallest = i;

	if (left <= heapSize && heap[left] < heap[i]) {
	    smallest = left;
	}
	if (right <= heapSize && heap[right] < heap[smallest]) {
	    smallest = right;
	}

	if (smallest != i) {
	    float tmp = heap[i];
	    heap[i] = heap[smallest];
	    heap[smallest] = tmp;
	    minHeapify(smallest);
	}
    }

    private int parent(int i) {
	return i/2;
    }
    private int left(int i) {
	return 2*i;
    }
    private int right(int i) {
	return 2*i+1;
    }
}
