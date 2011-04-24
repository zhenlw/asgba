#ifndef WORKER_H
#define WORKER_H

#include <wx/thread.h> // Base class: wxThread
#include <wx/event.h>

extern wxCriticalSection g_csWorker;
extern bool g_bStopWorker;

class Worker : public wxThread {
private:

public:
	Worker();
	~Worker();

	virtual void* Entry();
	void Stop();	//run in other thread
};

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_LOCAL_EVENT_TYPE(WORKER_NOTIFY_EVT, -1)
END_DECLARE_EVENT_TYPES()

#endif // WORKER_H
