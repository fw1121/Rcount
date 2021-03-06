// ==========================================================================
//                 SeqAn - The Library for Sequence Analysis
// ==========================================================================
// Copyright (c) 2006-2013, Knut Reinert, FU Berlin
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Knut Reinert or the FU Berlin nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL KNUT REINERT OR THE FU BERLIN BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// ==========================================================================
// Author: David Weese <david.weese@fu-berlin.de>
// ==========================================================================

#ifndef SEQAN_HEADER_STORE_IO_GFF_H
#define SEQAN_HEADER_STORE_IO_GFF_H

namespace SEQAN_NAMESPACE_MAIN {

//////////////////////////////////////////////////////////////////////////////
// Read Gff
//////////////////////////////////////////////////////////////////////////////

template <typename TFragmentStore, typename TSpec = void>
struct IOContextGff_
{
    typedef typename TFragmentStore::TAnnotationStore   TAnnotationStore;
    typedef typename Value<TAnnotationStore>::Type      TAnnotation;
    typedef typename TAnnotation::TId                   TId;

    CharString contigName;
    CharString typeName;
    CharString annotationName;
    CharString parentKey;
    CharString parentName;

    CharString _key;
    CharString _value;
    StringSet<CharString> keys;
    StringSet<CharString> values;

    CharString gtfGeneId;
    CharString gtfGeneName;
    CharString gtfTranscriptName;       // transcipt_id is stored in parentName

    TId annotationId;
    TAnnotation annotation;
};

template <typename TFragmentStore, typename TSpec>
inline void clear(IOContextGff_<TFragmentStore, TSpec> & ctx)
{
    typedef typename TFragmentStore::TAnnotationStore   TAnnotationStore;
    typedef typename Value<TAnnotationStore>::Type      TAnnotation;

    clear(ctx.contigName);
    clear(ctx.typeName);
    clear(ctx.annotationName);
    clear(ctx.parentKey);
    clear(ctx.parentName);
    clear(ctx._key);
    clear(ctx._value);
    clear(ctx.gtfGeneId);
    clear(ctx.gtfGeneName);
    clear(ctx.gtfTranscriptName);
    clear(ctx.keys);
    clear(ctx.values);
    ctx.annotationId = TAnnotation::INVALID_ID;
    clear(ctx.annotation.values);
}

//////////////////////////////////////////////////////////////////////////////
// _readOneAnnotation
//
// reads in one annotation line from a Gff file

template <typename TFragmentStore, typename TSpec>
inline void
_readOneAnnotation(
    IOContextGff_<TFragmentStore, TSpec> & ctx,
    GffRecord const & record)
{
//IOREV _nodoc_ _hasCRef_
    typedef typename TFragmentStore::TContigPos         TContigPos;

    clear(ctx);

    // read column 1: contig name
    ctx.contigName = record.ref;

    // skip column 2
    // read column 3: type
    ctx.typeName = record.type;

    // read column 4 and 5: begin and endposition
    ctx.annotation.beginPos = record.beginPos;
    ctx.annotation.endPos = record.endPos;

    // skip column 6
    // read column 7: orientation
    if (record.strand == '-')
    {
        TContigPos tmp = ctx.annotation.beginPos;
        ctx.annotation.beginPos = ctx.annotation.endPos;
        ctx.annotation.endPos = tmp;
    }

    // skip column 8
    // read column 9: name
    for (unsigned i = 0; i < length(record.tagName); ++i)
    {
        ctx._key = record.tagName[i];
        ctx._value = record.tagValue[i];
        if (ctx._key == "ID")
        {
            ctx.annotationName = ctx._value;
        }
        else if (!empty(ctx._key) && !empty(ctx._value))
        {
            appendValue(ctx.keys, ctx._key);
            appendValue(ctx.values, ctx._value);
        }

        if (ctx._key == "Parent" || ctx._key == "ParentID" || ctx._key == "transcript_id")
        {
            ctx.parentKey = ctx._key;
            ctx.parentName = ctx._value;
        }
        else if (ctx._key == "transcript_name")
        {
            ctx.gtfTranscriptName = ctx._value;
        }
        else if (ctx._key == "gene_id")
        {
            ctx.gtfGeneId = ctx._value;
        }
        else if (ctx._key == "gene_name")
        {
            ctx.gtfGeneName = ctx._value;
        }

        clear(ctx._key);
        clear(ctx._value);
    }
}

template <typename TAnnotation>
inline void
_adjustParent(
    TAnnotation & parent,
    TAnnotation const & child)
{
    if (child.contigId == TAnnotation::INVALID_ID || child.beginPos == TAnnotation::INVALID_POS || child.endPos == TAnnotation::INVALID_POS)
        return;

    parent.contigId = child.contigId;

    // Has parent an invalid begin and end position?
    if ((parent.beginPos == TAnnotation::INVALID_POS) && (parent.endPos == TAnnotation::INVALID_POS))
    {
        parent.beginPos = child.beginPos;
        parent.endPos = child.endPos;
        return;
    }

    if ((parent.beginPos == TAnnotation::INVALID_POS) || (parent.endPos == TAnnotation::INVALID_POS))
        return;

    typename TAnnotation::TPos childBegin, childEnd;
    if (child.beginPos < child.endPos)
    {
        childBegin = child.beginPos;
        childEnd = child.endPos;
    }
    else
    {
        childBegin = child.endPos;
        childEnd = child.beginPos;
    }

    // Keep parent's orientation and maximize begin and end using child's boundaries.
    if (parent.beginPos < parent.endPos)
    {
        if (parent.beginPos == TAnnotation::INVALID_POS || parent.beginPos > childBegin)
            parent.beginPos = childBegin;
        if (parent.endPos == TAnnotation::INVALID_POS || parent.endPos < childEnd)
            parent.endPos = childEnd;
    }
    else
    {
        if (parent.endPos == TAnnotation::INVALID_POS || parent.endPos > childBegin)
            parent.endPos = childBegin;
        if (parent.beginPos == TAnnotation::INVALID_POS || parent.beginPos < childEnd)
            parent.beginPos = childEnd;
    }
}

template <typename TFragmentStore, typename TSpec>
inline void
_storeOneAnnotation(
    TFragmentStore & fragStore,
    IOContextGff_<TFragmentStore, TSpec> & ctx)
{
    typedef typename TFragmentStore::TAnnotationStore   TAnnotationStore;
    typedef typename Value<TAnnotationStore>::Type      TAnnotation;
    typedef typename TAnnotation::TId                   TId;

    TId maxId = 0;

    // for lines in Gtf format get/add the parent gene first
    TId geneId = TAnnotation::INVALID_ID;
    if (!empty(ctx.gtfGeneId))
    {
        _storeAppendAnnotationName(fragStore, geneId, ctx.gtfGeneId, (TId) TFragmentStore::ANNO_GENE);
        if (maxId < geneId)
            maxId = geneId;
    }

    // if we have a parent transcript, get/add the parent transcript then
    if (!empty(ctx.parentName))
    {
// From now, we support gtf files with genes/transcripts having the same name.
//
//        // if gene and transcript names are equal (like in some strange gtf files)
//        // try to make the transcript name unique
//        if (ctx.gtfGeneId == ctx.parentName)
//            append(ctx.parentName, "_1");

        if (ctx.parentKey == "transcript_id")
        {
            // type is implicitly given (mRNA)
            _storeAppendAnnotationName(fragStore, ctx.annotation.parentId, ctx.parentName, (TId) TFragmentStore::ANNO_MRNA);
        }
        else
        {
            // type is unknown
            _storeAppendAnnotationName(fragStore, ctx.annotation.parentId, ctx.parentName);
        }
        if (maxId < ctx.annotation.parentId)
            maxId = ctx.annotation.parentId;
    }
    else
        ctx.annotation.parentId = 0;    // if we have no parent, we are a child of the root

    // add contig and type name
    _storeAppendContig(fragStore, ctx.annotation.contigId, ctx.contigName);
    _storeAppendType(fragStore, ctx.annotation.typeId, ctx.typeName);

    // add annotation name of the current line
    _storeAppendAnnotationName(fragStore, ctx.annotationId, ctx.annotationName, ctx.annotation.typeId);
    if (maxId < ctx.annotationId)
        maxId = ctx.annotationId;

    for (unsigned i = 0; i < length(ctx.keys); ++i)
    {
        // don't store gene_name as key/value pair unless it is a gene
        if (ctx.keys[i] == "gene_name" && ctx.annotation.typeId != TFragmentStore::ANNO_GENE)
            continue;

        // don't store transcript_name as key/value pair unless it is a transcript
        if (ctx.keys[i] == "transcript_name" && ctx.annotation.typeId != TFragmentStore::ANNO_MRNA)
            continue;

        // don't store Parent, transcript_id or gene_id as key/value pair (the are used to link annotations)
        if (ctx.keys[i] != ctx.parentKey && ctx.keys[i] != "gene_id")
            annotationAssignValueByKey(fragStore, ctx.annotation, ctx.keys[i], ctx.values[i]);
    }

    if (length(fragStore.annotationStore) <= maxId)
        resize(fragStore.annotationStore, maxId + 1, Generous());
    fragStore.annotationStore[ctx.annotationId] = ctx.annotation;

    TAnnotation & parent = fragStore.annotationStore[ctx.annotation.parentId];
    if (ctx.annotation.parentId != 0 && parent.parentId == TAnnotation::INVALID_ID)
        parent.parentId = 0;    // if our parent has no parent, it becomes a child of the root

    if (geneId != TAnnotation::INVALID_ID)
    {
        // link and adjust our gtf ancestors
        TAnnotation & gene = fragStore.annotationStore[geneId];
//        TAnnotation & transcript = fragStore.annotationStore[ctx.annotation.parentId];

        gene.parentId = 0;
        gene.typeId = TFragmentStore::ANNO_GENE;
        _adjustParent(gene, ctx.annotation);

        if (!empty(ctx.gtfGeneName))
            annotationAssignValueByKey(fragStore, gene, "gene_name", ctx.gtfGeneName);

        parent.parentId = geneId;
        parent.typeId = TFragmentStore::ANNO_MRNA;
        _adjustParent(parent, ctx.annotation);
        if (!empty(ctx.gtfTranscriptName))
            annotationAssignValueByKey(fragStore, parent, "transcript_name", ctx.gtfTranscriptName);
    }
}

///.Function.read.param.tag.type:Tag.File Format.tag.Gff
///.Function.read.param.tag.type:Tag.File Format.tag.Gtf
template <typename TFile, typename TSpec, typename TConfig>
inline void
read(
    TFile & file,
    FragmentStore<TSpec, TConfig> & fragStore,
    Gff)
{
    typedef FragmentStore<TSpec, TConfig> TFragmentStore;

    if (streamEof(file))
        return;

    IOContextGff_<TFragmentStore> ctx;

    refresh(fragStore.contigNameStoreCache);
    refresh(fragStore.annotationNameStoreCache);
    refresh(fragStore.annotationTypeStoreCache);

    RecordReader<TFile, SinglePass<> > reader(file);
    GffRecord record;
    while (!atEnd(reader))
    {
        if (!readRecord(record, reader, Gff()))
        {
            _readOneAnnotation(ctx, record);
            _storeOneAnnotation(fragStore, ctx);
        }
    }
    _storeClearAnnoBackLinks(fragStore.annotationStore);
    _storeCreateAnnoBackLinks(fragStore.annotationStore);
    _storeRemoveTempAnnoNames(fragStore);
}

template <typename TFile, typename TSpec, typename TConfig>
inline void
read(
    TFile & file,
    FragmentStore<TSpec, TConfig> & fragStore,
    Gtf)
{
    read(file, fragStore, Gff());
}

//////////////////////////////////////////////////////////////////////////////
// Write Gff
//////////////////////////////////////////////////////////////////////////////

// This function write the information that are equal for gff and gtf files.
template <typename TSpec, typename TConfig, typename TAnnotation, typename TId>
inline void
_writeCommonGffGtfInfo(
    GffRecord & record,
    FragmentStore<TSpec, TConfig> & store,
    TAnnotation & annotation,
    TId /*id*/)
{

    typedef FragmentStore<TSpec, TConfig>       TFragmentStore;
    typedef typename TFragmentStore::TContigPos TContigPos;

    // write column 1: contig name
    if (annotation.contigId < length(store.contigNameStore))
    {
        if (length(store.contigNameStore[annotation.contigId]) > 0u)
        {
            record.ref = store.contigNameStore[annotation.contigId];
        }
    }

    // skip column 2: source
    record.source = ".";

    // write column 3: type
    if (annotation.typeId < length(store.annotationTypeStore))
    {
        if (length(store.annotationTypeStore[annotation.typeId]) > 0u)
        {
            record.type = store.annotationTypeStore[annotation.typeId];
        }

    }

    TContigPos beginPos = annotation.beginPos;
    TContigPos endPos = annotation.endPos;
    char orientation = '+';
    if (endPos < beginPos)
    {
        TContigPos tmp = beginPos;
        beginPos = endPos;
        endPos = tmp;
        orientation = '-';
    }

    // write column 4: begin position
    if (beginPos != TAnnotation::INVALID_POS)
    {
        record.beginPos = beginPos;
    }

    // write column 5: end position
    if (endPos != TAnnotation::INVALID_POS)
    {
        record.endPos = endPos;
    }

    // skip column 6: score

    // write column 7: orientation
    record.strand = orientation;
}

template <typename TTargetStream, typename TSpec, typename TConfig, typename TAnnotation, typename TId>
inline bool
_writeOneAnnotation(
    TTargetStream & target,
    FragmentStore<TSpec, TConfig> & store,
    TAnnotation & annotation,
    TId id,
    Gff)
{

    if (id == 0)
        return false;

    GffRecord record;

    _writeCommonGffGtfInfo(record, store, annotation, id);

    // write column 9: group
    // write column 9.1: annotation id
    String<char> temp;
    if (id < length(store.annotationNameStore) && !empty(getAnnoName(store, id)))
    {
        appendValue(record.tagValue, getAnnoName(store, id));
    }
    else if (annotation.lastChildId != TAnnotation::INVALID_ID)
    {
        appendValue(record.tagValue, getAnnoUniqueName(store, id));
    }

    if (length(record.tagValue[0]) > 0)
    {
        appendValue(record.tagName, "ID");
    }

    // write column 9.2: parent id
    if (store.annotationStore[annotation.parentId].typeId > 1)  // ignore root/deleted nodes
    {
        appendValue(record.tagName, "Parent");
        appendValue(record.tagValue, getAnnoUniqueName(store, annotation.parentId));
    }

    // write column 9.3-...: key, value pairs
    for (unsigned keyId = 0; keyId < length(annotation.values); ++keyId)
        if (!empty(annotation.values[keyId]))
        {
            appendValue(record.tagName, store.annotationKeyStore[keyId]);
            appendValue(record.tagValue, annotation.values[keyId]);
        }

    return writeRecord(target, record, Gff());
}

template <typename TTargetStream, typename TSpec, typename TConfig, typename TAnnotation, typename TId>
inline bool
_writeOneAnnotation(
    TTargetStream & target,
    FragmentStore<TSpec, TConfig> & store,
    TAnnotation & annotation,
    TId id,
    Gtf)
{
    typedef FragmentStore<TSpec, TConfig>               TFragmentStore;

    if (annotation.typeId <= TFragmentStore::ANNO_MRNA)
        return false;

    GffRecord record;

    _writeCommonGffGtfInfo(record, store, annotation, id);

    // write column 9: group

    // step up until we reach a transcript
    TId transcriptId = annotation.parentId;
    while (transcriptId < length(store.annotationStore) && store.annotationStore[transcriptId].typeId != TFragmentStore::ANNO_MRNA)
        transcriptId = store.annotationStore[transcriptId].parentId;

    // step up until we reach a gene
    TId geneId = transcriptId;
    while (geneId < length(store.annotationStore) && store.annotationStore[geneId].typeId != TFragmentStore::ANNO_GENE)
        geneId = store.annotationStore[geneId].parentId;

    CharString tmpStr;
    if (geneId < length(store.annotationStore) && annotationGetValueByKey(store, store.annotationStore[geneId], "gene_name", tmpStr))
    {
        appendValue(record.tagName, "gene_name");
        appendValue(record.tagValue, tmpStr);
    }
    if (transcriptId < length(store.annotationStore) && annotationGetValueByKey(store, store.annotationStore[transcriptId], "transcript_name", tmpStr))
    {
        appendValue(record.tagName, "transcript_name");
        appendValue(record.tagValue, tmpStr);
    }

    if (id < length(store.annotationNameStore) && !empty(getAnnoName(store, id)))
    {
        appendValue(record.tagName, "ID");
        appendValue(record.tagValue, getAnnoName(store, id));
    }

    // write key, value pairs
    for (unsigned keyId = 0; keyId < length(annotation.values); ++keyId)
        if (!empty(annotation.values[keyId]))
        {
            appendValue(record.tagName, store.annotationKeyStore[keyId]);
            appendValue(record.tagValue, annotation.values[keyId]);
        }

    // The GTF format version 2.2 requires the keys gene_id and transcript_id to be the last keys of line
    // read http://mblab.wustl.edu/GTF22.html and http://www.bioperl.org/wiki/GTF

    if (geneId < length(store.annotationStore))
    {
        appendValue(record.tagName, "gene_id");
        appendValue(record.tagValue, getAnnoUniqueName(store, geneId));
    }

    if (transcriptId < length(store.annotationStore))
    {
        appendValue(record.tagName, "transcript_id");
        appendValue(record.tagValue, getAnnoUniqueName(store, transcriptId));
    }

    return writeRecord(target, record, Gtf());
}

template <typename TTargetStream, typename TSpec, typename TConfig, typename TFormat>
inline void
_writeGffGtf(
    TTargetStream & target,
    FragmentStore<TSpec, TConfig> & store,
    TFormat format)
{
    typedef FragmentStore<TSpec, TConfig>                           TFragmentStore;
    typedef typename TFragmentStore::TAnnotationStore               TAnnotationStore;
    typedef typename Value<TAnnotationStore>::Type                  TAnnotation;
    typedef typename Iterator<TAnnotationStore, Standard>::Type     TAnnoIter;
    typedef typename Id<TAnnotation>::Type                          TId;

    TAnnoIter it = begin(store.annotationStore, Standard());
    TAnnoIter itEnd = end(store.annotationStore, Standard());

    for (TId id = 0; it != itEnd; ++it, ++id)
        _writeOneAnnotation(target, store, *it, id, format);
}

template <typename TTargetStream, typename TSpec, typename TConfig>
inline void
write(
    TTargetStream & target,
    FragmentStore<TSpec, TConfig> & store,
    Gff format)
{
    _writeGffGtf(target, store, format);
}

template <typename TTargetStream, typename TSpec, typename TConfig>
inline void
write(
    TTargetStream & target,
    FragmentStore<TSpec, TConfig> & store,
    Gtf format)
{
    _writeGffGtf(target, store, format);
}

} // namespace SEQAN_NAMESPACE_MAIN

#endif //#ifndef SEQAN_HEADER_...
