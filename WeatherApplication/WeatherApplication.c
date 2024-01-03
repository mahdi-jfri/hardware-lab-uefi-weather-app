#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/Http.h>
#include <Protocol/ServiceBinding.h>

#define BUFFER_SIZE 0x100000


BOOLEAN gRequestCallbackComplete = FALSE;
BOOLEAN gResponseCallbackComplete = FALSE;

VOID EFIAPI RequestCallback(IN EFI_EVENT Event, IN VOID *Context) {
    gRequestCallbackComplete = TRUE;
}

VOID EFIAPI ResponseCallback(IN EFI_EVENT Event, IN VOID *Context) {
    gResponseCallbackComplete = TRUE;
}

UINTN GetResponseCode(EFI_HTTP_STATUS_CODE code) {
    switch (code) {
        case HTTP_STATUS_100_CONTINUE: return 100; 
        case HTTP_STATUS_101_SWITCHING_PROTOCOLS: return 101; 
        case HTTP_STATUS_200_OK: return 200; 
        case HTTP_STATUS_201_CREATED: return 201; 
        case HTTP_STATUS_202_ACCEPTED: return 202; 
        case HTTP_STATUS_203_NON_AUTHORITATIVE_INFORMATION: return 203; 
        case HTTP_STATUS_204_NO_CONTENT: return 204; 
        case HTTP_STATUS_205_RESET_CONTENT: return 205; 
        case HTTP_STATUS_206_PARTIAL_CONTENT: return 206; 
        case HTTP_STATUS_300_MULTIPLE_CHOICES: return 300; 
        case HTTP_STATUS_301_MOVED_PERMANENTLY: return 301; 
        case HTTP_STATUS_302_FOUND: return 302; 
        case HTTP_STATUS_303_SEE_OTHER: return 303; 
        case HTTP_STATUS_304_NOT_MODIFIED: return 304; 
        case HTTP_STATUS_305_USE_PROXY: return 305; 
        case HTTP_STATUS_307_TEMPORARY_REDIRECT: return 307; 
        case HTTP_STATUS_400_BAD_REQUEST: return 400; 
        case HTTP_STATUS_401_UNAUTHORIZED: return 401; 
        case HTTP_STATUS_402_PAYMENT_REQUIRED: return 402; 
        case HTTP_STATUS_403_FORBIDDEN: return 403; 
        case HTTP_STATUS_404_NOT_FOUND: return 404; 
        case HTTP_STATUS_405_METHOD_NOT_ALLOWED: return 405; 
        case HTTP_STATUS_406_NOT_ACCEPTABLE: return 406; 
        case HTTP_STATUS_407_PROXY_AUTHENTICATION_REQUIRED: return 407; 
        case HTTP_STATUS_408_REQUEST_TIME_OUT: return 408; 
        case HTTP_STATUS_409_CONFLICT: return 409; 
        case HTTP_STATUS_410_GONE: return 410; 
        case HTTP_STATUS_411_LENGTH_REQUIRED: return 411; 
        case HTTP_STATUS_412_PRECONDITION_FAILED: return 412; 
        case HTTP_STATUS_413_REQUEST_ENTITY_TOO_LARGE: return 413; 
        case HTTP_STATUS_414_REQUEST_URI_TOO_LARGE: return 414; 
        case HTTP_STATUS_415_UNSUPPORTED_MEDIA_TYPE: return 415; 
        case HTTP_STATUS_416_REQUESTED_RANGE_NOT_SATISFIED: return 416; 
        case HTTP_STATUS_417_EXPECTATION_FAILED: return 417; 
        case HTTP_STATUS_500_INTERNAL_SERVER_ERROR: return 500; 
        case HTTP_STATUS_501_NOT_IMPLEMENTED: return 501; 
        case HTTP_STATUS_502_BAD_GATEWAY: return 502; 
        case HTTP_STATUS_503_SERVICE_UNAVAILABLE: return 503; 
        case HTTP_STATUS_504_GATEWAY_TIME_OUT: return 504; 
        case HTTP_STATUS_505_HTTP_VERSION_NOT_SUPPORTED: return 505; 
        case HTTP_STATUS_308_PERMANENT_REDIRECT: return 308; 
        default: return 0;
    }
}

/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS Status;
    EFI_HTTP_PROTOCOL *Http = NULL;
    EFI_SERVICE_BINDING_PROTOCOL *ServiceBinding = NULL;
    EFI_HANDLE Handle = NULL;
    EFI_HTTP_CONFIG_DATA ConfigData;
    EFI_HTTPv4_ACCESS_POINT IPv4Node;
    EFI_HTTP_REQUEST_DATA RequestData;
    EFI_HTTP_HEADER RequestHeaders[2];
    EFI_HTTP_MESSAGE RequestMessage;
    EFI_HTTP_TOKEN RequestToken;
    EFI_HTTP_RESPONSE_DATA ResponseData;
    EFI_HTTP_MESSAGE ResponseMessage;
    EFI_HTTP_TOKEN ResponseToken;
    UINT8 *Buffer;
    EFI_TIME Baseline;
    EFI_TIME Current;
    UINTN Timer;
    UINTN ContentLength;
    UINTN ContentDownloaded;
    UINTN Index;

    // Locate the HTTP protocol
    Status = gBS->AllocatePool(EfiBootServicesData, BUFFER_SIZE, (VOID **)&Buffer);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to allocate pool: %r\n", Status);
        return Status;
    }

    Status = gBS->LocateProtocol(&gEfiHttpServiceBindingProtocolGuid, NULL, (VOID **)&ServiceBinding);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate binding protocol: %r\n", Status);
        return Status;
    }

    Status = ServiceBinding->CreateChild(ServiceBinding, &Handle);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to create child: %r\n", Status);
        return Status;
    }

    Status = gBS->HandleProtocol(Handle, &gEfiHttpProtocolGuid, (VOID **)&Http);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to handle HTTP protocol: %r\n", Status);
        return Status;
    }

    ConfigData.HttpVersion = HttpVersion11;
    ConfigData.TimeOutMillisec = 0;
    ConfigData.LocalAddressIsIPv6 = FALSE;

    ZeroMem(&IPv4Node, sizeof(IPv4Node));
    IPv4Node.UseDefaultAddress = TRUE;
    IPv4Node.LocalPort = 6349;
    ConfigData.AccessPoint.IPv4Node = &IPv4Node;

    Status = Http->Configure(Http, &ConfigData);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to configure HTTP driver: %r\n", Status);
        return Status;
    }

    RequestData.Url = L"http://httpbin.org/get";
    RequestData.Method = HttpMethodGet;

    RequestHeaders[0].FieldName = "Host";
    RequestHeaders[0].FieldValue = "httpbin.org";
    RequestHeaders[1].FieldName = "Accept-Encoding";
    RequestHeaders[1].FieldValue = "identity";

    RequestMessage.Data.Request = &RequestData;
    RequestMessage.HeaderCount = 2;
    RequestMessage.Headers = RequestHeaders;
    RequestMessage.BodyLength = 0;
    RequestMessage.Body = NULL;
    
    RequestToken.Event = NULL;
    Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, RequestCallback, NULL, &RequestToken.Event);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to create event: %r\n", Status);
        return Status;
    }

    RequestToken.Status = EFI_SUCCESS;
    RequestToken.Message = &RequestMessage;
    gRequestCallbackComplete = FALSE;

    Status = Http->Request(Http, &RequestToken);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to send HTTP request: %r\n", Status);
        return Status;
    }

    Status = gRT->GetTime(&Baseline, NULL);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get baseline time: %r\n", Status);
        return Status;
    }

    for (Timer = 0; !gRequestCallbackComplete && Timer < 10;) {
        Http->Poll(Http);
        if (!EFI_ERROR(gRT->GetTime(&Current, NULL)) && Current.Second != Baseline.Second) {
            Baseline = Current;
            ++Timer;
        }
    }

    if (!gRequestCallbackComplete) {
        Status = Http->Cancel(Http, &RequestToken);
        Print(L"Request timed out.");
        return Status;
    }

    ResponseData.StatusCode = HTTP_STATUS_UNSUPPORTED_STATUS;
    ResponseMessage.Data.Response = &ResponseData;
    // HeaderCount will be updated by the HTTP driver on response.
    ResponseMessage.HeaderCount = 0;
    // Headers will be populated by the driver on response.
    ResponseMessage.Headers = NULL;
    // BodyLength maximum limit is defined by the caller. On response,
    // the HTTP driver will update BodyLength to the total number of
    // bytes copied to Body. This number will never exceed the initial
    // maximum provided by the caller.
    ResponseMessage.BodyLength = BUFFER_SIZE;
    ResponseMessage.Body = Buffer;
    // Token format is similar to the token format in EFI TCP protocol.
    ResponseToken.Event = NULL;
    Status = gBS->CreateEvent(
        EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        ResponseCallback,
        &ResponseToken,
        &ResponseToken.Event
    );
    ResponseToken.Status = EFI_SUCCESS;
    ResponseToken.Message = &ResponseMessage;

    gResponseCallbackComplete = FALSE;
    // Finally, make HTTP request.
    Status = Http->Response(Http, &ResponseToken);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to receive HTTP response: %r\n", Status);
        return Status;
    }
    Status = gRT->GetTime(&Baseline, NULL);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get baseline time for response: %r\n", Status);
        return Status;
    }
    // Optionally, wait for a certain amount of time before cancelling.
    for (Timer = 0; !gResponseCallbackComplete && Timer < 10; ) {
        Http->Poll(Http);
        if (!EFI_ERROR(gRT->GetTime(&Current, NULL)) && Current.Second != Baseline.Second)
        {
            // One second has passed, so update Current time and
            // increment the counter.
            Baseline = Current;
            ++Timer;
        }
    }
    // Remove response token from queue if we did not get a notification
    // from the remote host in a timely manner.
    if (!gResponseCallbackComplete) {
        Status = Http->Cancel(Http, &ResponseToken);
        Print(L"Response timed out.");
        return Status;
    }
    Print(L"Status Code: %d\n", ResponseData.StatusCode);
    Print(L"Status Code Real: %d\n", GetResponseCode(ResponseData.StatusCode));
    
    for (Index = 0; Index < ResponseMessage.HeaderCount; ++Index) {
        // We can parse the length of the file from the ContentLength header.
        if (!AsciiStriCmp(ResponseMessage.Headers[Index].FieldName, "Content-Length")) {
            ContentLength = AsciiStrDecimalToUintn(ResponseMessage.Headers[Index].FieldValue);
        }
    }

    Print(L"Content Length: %d\n", ContentLength);
    for(Index = 0; Index < ResponseMessage.BodyLength; ++Index) {
        Print(L"%c", Buffer[Index]);
    }

    ContentDownloaded = ResponseMessage.BodyLength;

    while (ContentDownloaded < ContentLength) {
        // If we make it here, we haven't yet downloaded the whole file and
        // need to keep going.
        ResponseMessage.Data.Response = NULL;
        if (ResponseMessage.Headers != NULL) {
            // No sense hanging onto this anymore.
            // gBS->FreePool(ResponseMessage.Headers);
        }
        ResponseMessage.HeaderCount = 0;
        ResponseMessage.BodyLength = BUFFER_SIZE;
        ZeroMem(Buffer, BUFFER_SIZE);
        // ResponseMessage.Body still points to Buffer.
        gResponseCallbackComplete = FALSE;
        // The HTTP driver accepts a token where Data, Headers, and
        // HeaderCount are all 0 or NULL. The driver will wait for a
        // response from the last remote host which a transaction occurred
        // and copy the response directly into Body, updating BodyLength
        // with the total amount copied (downloaded).
        Status = Http->Response(Http, &ResponseToken);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to receive HTTP response: %r\n", Status);
            return Status;
        }
        Status = gRT->GetTime(&Baseline, NULL);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to get baseline time for response: %r\n", Status);
            return Status;
        }
        for (Timer = 0; !gResponseCallbackComplete && Timer < 10; ) {
            Http->Poll(Http);
            if (!EFI_ERROR(gRT->GetTime(&Current, NULL)) && Current.Second != Baseline.Second)
            {
                // One second has passed, so update Current time and
                // increment the counter.
                Baseline = Current;
                ++Timer;
            }
        }
        // Remove response token from queue if we did not get a notification
        // from the remote host in a timely manner.
        if (!gResponseCallbackComplete) {
            Status = Http->Cancel(Http, &ResponseToken);
            Print(L"Response timed out.");
            return Status;
        }
        // Assuming we successfully received a response...
        ContentDownloaded += ResponseMessage.BodyLength;
        for(Index = 0; Index < ResponseMessage.BodyLength; ++Index) {
            Print(L"%c", Buffer[Index]);
        }
    }
    Print(L"\n");

    Print(L"Response status code: %d\n", GetResponseCode(ResponseData.StatusCode));

    Status = ServiceBinding->DestroyChild(ServiceBinding, Handle);

    if (EFI_ERROR(Status)) {
        Print(L"Failed to destroy binding service: %r\n", Status);
        return Status;
    }

    return EFI_SUCCESS;
}

