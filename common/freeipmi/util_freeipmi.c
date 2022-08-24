#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "util_freeipmi.h"

/*
  - Name: freeipmi_session_create
  - Description: Create an session for freeipmi
  - Input:
      * none
  - Return:
      * Pointer to store free-ipmi session, if no error
      * NULL, if error
*/
ipmi_ctx_t freeipmi_session_create()
{
    ipmi_ctx_t ipmi_ctx = ipmi_ctx_create();
    if (ipmi_ctx == NULL) {
        log_print(LOG_ERR, "%s: ipmi_ctx_create error\n", __func__);
        return ipmi_ctx;
    }

    ipmi_ctx->type = IPMI_DEVICE_OPENIPMI;
    if (!(ipmi_ctx->io.inband.openipmi_ctx = ipmi_openipmi_ctx_create ())) {
        log_print(LOG_ERR, "%s: !(ipmi_ctx->io.inband.openipmi_ctx = ipmi_openipmi_ctx_create ())\n", __func__);
        goto clean;
    }

    if (ipmi_openipmi_ctx_io_init (ipmi_ctx->io.inband.openipmi_ctx) < 0) {
        log_print(LOG_ERR, "%s: ipmi_openipmi_ctx_io_init (ctx->io.inband.openipmi_ctx) < 0\n", __func__);
        goto clean;
    }

    return ipmi_ctx;

clean:
    freeipmi_session_abort(ipmi_ctx);
    ipmi_ctx = NULL;
    return ipmi_ctx;
}

/*
  - Name: freeipmi_session_abort
  - Description: Abort freeipmi session
  - Input:
      * ipmi_ctx: pointer to save ipmi-raw session
  - Return:
      * 0, no error
      * 1, if error
*/
int freeipmi_session_abort(ipmi_ctx_t ipmi_ctx)
{
    PARM_CHECK(ipmi_ctx, 1, __func__)

    ipmi_ctx_close (ipmi_ctx);
    ipmi_ctx_destroy (ipmi_ctx);

    return 0;
}
