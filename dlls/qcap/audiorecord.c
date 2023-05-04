/*
 * Audio capture filter
 *
 * Copyright 2015 Damjan Jovanovic
 * Copyright 2023 Zeb Figura for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "qcap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct audio_record
{
    struct strmbase_filter filter;
    IPersistPropertyBag IPersistPropertyBag_iface;

    struct strmbase_source source;
};

static struct audio_record *impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, struct audio_record, filter);
}

static HRESULT WINAPI audio_record_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props)
{
    ALLOCATOR_PROPERTIES ret_props;

    return IMemAllocator_SetProperties(allocator, props, &ret_props);
}

static const struct strmbase_source_ops source_ops =
{
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = audio_record_source_DecideBufferSize,
};

static struct audio_record *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct audio_record, IPersistPropertyBag_iface);
}

static struct strmbase_pin *audio_record_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (!index)
        return &filter->source.pin;
    return NULL;
}

static void audio_record_destroy(struct strmbase_filter *iface)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT audio_record_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = audio_record_get_pin,
    .filter_destroy = audio_record_destroy,
    .filter_query_interface = audio_record_query_interface,
};

static HRESULT WINAPI PPB_QueryInterface(IPersistPropertyBag *iface, REFIID riid, LPVOID *ppv)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, ppv);
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag *iface)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag *iface)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI PPB_GetClassID(IPersistPropertyBag *iface, CLSID *pClassID)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, pClassID);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag *iface)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(): stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_Load(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        IErrorLog *pErrorLog)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    HRESULT hr;
    VARIANT var;

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pPropBag, pErrorLog);

    V_VT(&var) = VT_I4;
    hr = IPropertyBag_Read(pPropBag, L"WaveInID", &var, pErrorLog);
    if (SUCCEEDED(hr))
    {
        FIXME("FIXME: implement opening waveIn device %ld\n", V_I4(&var));
    }

    return hr;
}

static HRESULT WINAPI PPB_Save(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        BOOL fClearDirty, BOOL fSaveAllProperties)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(%p, %u, %u): stub\n", iface, This, pPropBag, fClearDirty, fSaveAllProperties);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl PersistPropertyBagVtbl =
{
    PPB_QueryInterface,
    PPB_AddRef,
    PPB_Release,
    PPB_GetClassID,
    PPB_InitNew,
    PPB_Load,
    PPB_Save
};

HRESULT audio_record_create(IUnknown *outer, IUnknown **out)
{
    struct audio_record *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;
    strmbase_filter_init(&object->filter, outer, &CLSID_AudioRecord, &filter_ops);

    strmbase_source_init(&object->source, &object->filter, L"Capture", &source_ops);

    TRACE("Created audio recorder %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
