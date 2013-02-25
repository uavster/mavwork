#ifndef __FILTER_INCLUDED__
#define __FILTER_INCLUDED__

class Filter {
	int order;
	int trigger;
	float *num;
	float *den;
	float *sampleHistory;
	float *outputHistory;
	int index;

	inline int mod(int a, int b);
	void destroy();
public:
	Filter();
	Filter(Filter &f);
	~Filter();

	void create(int order, float *num, float *den);
	float filter(float currentSample);
	inline void setTrigger(int trigger) { this->trigger = trigger; }
};

#endif
