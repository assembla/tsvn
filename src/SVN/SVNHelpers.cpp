#include "StdAfx.h"
#include "SVNHelpers.h"


SVNPool::SVNPool()
{
	m_pool = svn_pool_create(NULL);
}

SVNPool::SVNPool(apr_pool_t* parentPool)
{
	m_pool = svn_pool_create(parentPool);
}

SVNPool::~SVNPool()
{
	svn_pool_destroy(m_pool);
}

SVNPool::operator apr_pool_t*()
{
	return m_pool;
}


// The time is not yet right for this base class, but I'm thinking about it...

SVNHelper::SVNHelper(void)
{
	m_pool = svn_pool_create (NULL);				// create the memory pool
	
	svn_utf_initialize(m_pool);

	// Why do we do this?
	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", m_pool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", m_pool);

	svn_error_t* err = svn_client_create_context(&m_ctx, m_pool);

}

SVNHelper::~SVNHelper(void)
{
	svn_pool_destroy (m_pool);
}


static class SVNHelperTests
{
public:
	SVNHelperTests()
	{
		apr_initialize();
		{
			SVNHelper base;
		}
		apr_terminate();
	}

} SVNHelperTestsInstance;

