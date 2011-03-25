#ifndef WORKER_H
#define WORKER_H

#include <wx/thread.h> // Base class: wxThread
#include <wx/event.h>

class Worker : public wxThread {

public:
	Worker();
	~Worker();

public:
	virtual void* Entry();
};

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_LOCAL_EVENT_TYPE(WORKER_NOTIFY_EVT, -1)
END_DECLARE_EVENT_TYPES()

#endif // WORKER_H
