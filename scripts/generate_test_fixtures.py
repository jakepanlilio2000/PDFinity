import os, hashlib, shutil, subprocess

def create_fixtures():
    fixtures_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "tests", "fixtures"))
    os.makedirs(fixtures_dir, exist_ok=True)

    # 1. Ordinary PDF (2 pages, standard metadata)
    doc_ordinary = (
        b"%PDF-1.7\r\n"
        b"1 0 obj\r\n<< /Type /Catalog /Pages 2 0 R >>\r\nendobj\r\n"
        b"2 0 obj\r\n<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 2 >>\r\nendobj\r\n"
        b"3 0 obj\r\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 5 0 R >>\r\nendobj\r\n"
        b"4 0 obj\r\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Contents 6 0 R >>\r\nendobj\r\n"
        b"5 0 obj\r\n<< /Length 44 >>\r\nstream\r\nBT /F1 12 Tf 72 712 Td (Page 1 Content) Tj ET\r\nendstream\r\nendobj\r\n"
        b"6 0 obj\r\n<< /Length 44 >>\r\nstream\r\nBT /F1 12 Tf 72 762 Td (Page 2 Content) Tj ET\r\nendstream\r\nendobj\r\n"
        b"7 0 obj\r\n<< /Title (PDFinity Ordinary Document) /Author (Lead C++ Architect) /Subject (Integration Testing) /Keywords (pdf, test, pdfinity) >>\r\nendobj\r\n"
        b"xref\r\n"
        b"0 8\r\n"
        b"0000000000 65535 f \r\n"
        b"0000000010 00000 n \r\n"
        b"0000000059 00000 n \r\n"
        b"0000000118 00000 n \r\n"
        b"0000000201 00000 n \r\n"
        b"0000000284 00000 n \r\n"
        b"0000000377 00000 n \r\n"
        b"0000000470 00000 n \r\n"
        b"trailer\r\n<< /Size 8 /Root 1 0 R /Info 7 0 R >>\r\nstartxref\r\n600\r\n%%EOF\r\n"
    )
    with open(os.path.join(fixtures_dir, "ordinary.pdf"), "wb") as f:
        f.write(doc_ordinary)

    # 2. Unicode Metadata PDF (UTF-16BE with BOM \xfe\xff)
    title_u16 = b"\xfe\xff" + "PDFinity ドキュメント 2026 (日本語)".encode("utf-16-be")
    author_u16 = b"\xfe\xff" + "Александр Пушкин".encode("utf-16-be")

    def escape_bytes(b):
        res = bytearray()
        for byte in b:
            if byte in (ord('\\'), ord('('), ord(')')):
                res.extend(b'\\' + bytes([byte]))
            else:
                res.append(byte)
        return bytes(res)

    doc_unicode = (
        b"%PDF-1.7\r\n"
        b"1 0 obj\r\n<< /Type /Catalog /Pages 2 0 R >>\r\nendobj\r\n"
        b"2 0 obj\r\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\r\nendobj\r\n"
        b"3 0 obj\r\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] >>\r\nendobj\r\n"
        b"4 0 obj\r\n<< /Title (" + escape_bytes(title_u16) + b") /Author (" + escape_bytes(author_u16) + b") >>\r\nendobj\r\n"
        b"xref\r\n"
        b"0 5\r\n"
        b"0000000000 65535 f \r\n"
        b"0000000010 00000 n \r\n"
        b"0000000059 00000 n \r\n"
        b"0000000110 00000 n \r\n"
        b"0000000185 00000 n \r\n"
        b"trailer\r\n<< /Size 5 /Root 1 0 R /Info 4 0 R >>\r\nstartxref\r\n350\r\n%%EOF\r\n"
    )
    with open(os.path.join(fixtures_dir, "unicode_metadata.pdf"), "wb") as f:
        f.write(doc_unicode)

    # 3. Image-heavy PDF (Embedded 16x16 RGB image XObject)
    raw_img = bytearray()
    for y in range(16):
        for x in range(16):
            if (x // 2 + y // 2) % 2 == 0:
                raw_img.extend([255, 0, 0]) # Red
            else:
                raw_img.extend([0, 0, 255]) # Blue

    img_len = len(raw_img)
    doc_image = (
        b"%PDF-1.7\r\n"
        b"1 0 obj\r\n<< /Type /Catalog /Pages 2 0 R >>\r\nendobj\r\n"
        b"2 0 obj\r\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\r\nendobj\r\n"
        b"3 0 obj\r\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 200] /Resources << /XObject << /Im1 4 0 R >> >> /Contents 5 0 R >>\r\nendobj\r\n"
        b"4 0 obj\r\n<< /Type /XObject /Subtype /Image /Width 16 /Height 16 /ColorSpace /DeviceRGB /BitsPerComponent 8 /Length " + str(img_len).encode() + b" >>\r\nstream\r\n" +
        bytes(raw_img) + b"\r\nendstream\r\nendobj\r\n"
        b"5 0 obj\r\n<< /Length 38 >>\r\nstream\r\nq 100 0 0 100 50 50 cm /Im1 Do Q\r\nendstream\r\nendobj\r\n"
        b"xref\r\n"
        b"0 6\r\n"
        b"0000000000 65535 f \r\n"
        b"0000000010 00000 n \r\n"
        b"0000000059 00000 n \r\n"
        b"0000000110 00000 n \r\n"
        b"0000000220 00000 n \r\n"
        b"0000001050 00000 n \r\n"
        b"trailer\r\n<< /Size 6 /Root 1 0 R >>\r\nstartxref\r\n1140\r\n%%EOF\r\n"
    )
    with open(os.path.join(fixtures_dir, "image_heavy.pdf"), "wb") as f:
        f.write(doc_image)

    # 4. Encrypted PDF (Verified standard V1/R2 40-bit RC4 encryption with password "testpass123")
    pad = b"\x28\xbf\x4e\x5e\x4e\x75\x8a\x41\x64\x00\x4e\x56\xff\xfa\x01\x08\x2e\x2e\x00\xb6\xd0\x68\x3e\x80\x2f\x0c\xa9\xfe\x64\x53\x69\x7a"
    def rc4_crypt(key, data):
        S = list(range(256))
        j = 0
        out = bytearray()
        for i in range(256):
            j = (j + S[i] + key[i % len(key)]) % 256
            S[i], S[j] = S[j], S[i]
        i = j = 0
        for b in data:
            i = (i + 1) % 256
            j = (j + S[i]) % 256
            S[i], S[j] = S[j], S[i]
            out.append(b ^ S[(S[i] + S[j]) % 256])
        return bytes(out)

    user_pwd = b"testpass123"
    padded_user = (user_pwd + pad)[:32]
    o_key = hashlib.md5(padded_user).digest()[:5]
    o_val = rc4_crypt(o_key, padded_user)

    file_id = b"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10"
    p_val = -4
    p_bytes = p_val.to_bytes(4, byteorder='little', signed=True)

    h = hashlib.md5()
    h.update(padded_user)
    h.update(o_val)
    h.update(p_bytes)
    h.update(file_id)
    enc_key = h.digest()[:5]
    u_val = rc4_crypt(enc_key, pad)

    cat_body = b"<< /Type /Catalog /Pages 2 0 R >>"
    pages_body = b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>"
    page_body = b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] >>"

    doc_enc = (
        b"%PDF-1.4\r\n"
        b"1 0 obj\r\n" + cat_body + b"\r\nendobj\r\n"
        b"2 0 obj\r\n" + pages_body + b"\r\nendobj\r\n"
        b"3 0 obj\r\n" + page_body + b"\r\nendobj\r\n"
        b"4 0 obj\r\n<< /Filter /Standard /V 1 /R 2 /O <" + o_val.hex().encode() + b"> /U <" + u_val.hex().encode() + b"> /P -4 >>\r\nendobj\r\n"
        b"xref\r\n0 5\r\n0000000000 65535 f \r\n0000000010 00000 n \r\n0000000059 00000 n \r\n0000000115 00000 n \r\n0000000185 00000 n \r\n"
        b"trailer\r\n<< /Size 5 /Root 1 0 R /Encrypt 4 0 R /ID [<0102030405060708090a0b0c0d0e0f10> <0102030405060708090a0b0c0d0e0f10>] >>\r\n"
        b"startxref\r\n350\r\n%%EOF\r\n"
    )
    with open(os.path.join(fixtures_dir, "encrypted_user_pass.pdf"), "wb") as f:
        f.write(doc_enc)

    # 5. Malformed PDFs
    with open(os.path.join(fixtures_dir, "malformed_corrupt_header.pdf"), "wb") as f:
        f.write(b"NOT_A_PDF_FILE_HEADER_GARBAGE_1234567890\r\nSome random corrupted text...")

    with open(os.path.join(fixtures_dir, "malformed_corrupt_xref.pdf"), "wb") as f:
        f.write(b"%PDF-1.7\r\n1 0 obj\r\n<< /Type /Catalog >>\r\nendobj\r\nxref\r\n0 1\r\nINVALID_XREF_ENTRIES_BROKEN\r\n%%EOF")

    with open(os.path.join(fixtures_dir, "malformed_empty.pdf"), "wb") as f:
        pass # 0 bytes

    # 6. Bookmarked & Searchable PDF (Dynamic helper for accurate offsets)
    class SimplePdf:
        def __init__(self):
            self.objs = []
        def add(self, b):
            self.objs.append(b)
            return len(self.objs)
        def write(self, root_id, info_id=None):
            out = bytearray(b"%PDF-1.7\r\n%\xe2\xe3\xcf\xd3\r\n")
            offs = [0]
            for i, o in enumerate(self.objs, 1):
                offs.append(len(out))
                out.extend(f"{i} 0 obj\r\n".encode() + o + b"\r\nendobj\r\n")
            xpos = len(out)
            out.extend(f"xref\r\n0 {len(offs)}\r\n0000000000 65535 f \r\n".encode())
            for off in offs[1:]:
                out.extend(f"{off:010d} 00000 n \r\n".encode())
            inf = f" /Info {info_id} 0 R" if info_id else ""
            out.extend(f"trailer\r\n<< /Size {len(offs)} /Root {root_id} 0 R{inf} >>\r\nstartxref\r\n{xpos}\r\n%%EOF\r\n".encode())
            return bytes(out)

    sp = SimplePdf()
    # 1: Font
    font_id = sp.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>")
    # 2: Content page 1
    p1_text = (b"BT /F1 14 Tf 50 720 Td (PDFinity Architecture Overview) Tj "
               b"/F1 11 Tf 0 -24 Td (PDFinity is a modular offline-first PDF productivity suite.) Tj "
               b"0 -20 Td (It supports multi-threaded asynchronous rendering and fast text search.) Tj ET")
    c1_id = sp.add(f"<< /Length {len(p1_text)} >>\r\nstream\r\n".encode() + p1_text + b"\r\nendstream")
    # 3: Content page 2
    p2_text = (b"BT /F1 14 Tf 50 720 Td (Cross-Platform Systems Engineering) Tj "
               b"/F1 11 Tf 0 -24 Td (High-assurance PDF processing requires bounded memory caches.) Tj "
               b"0 -20 Td (Search query matching supports exact case and whole-word modes.) Tj ET")
    c2_id = sp.add(f"<< /Length {len(p2_text)} >>\r\nstream\r\n".encode() + p2_text + b"\r\nendstream")
    # 4: Page 1, 5: Page 2 (forward declared IDs)
    page1_id = 4
    page2_id = 5
    # Add pages
    sp.add(f"<< /Type /Page /Parent 6 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {font_id} 0 R >> >> /Contents {c1_id} 0 R >>".encode())
    sp.add(f"<< /Type /Page /Parent 6 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {font_id} 0 R >> >> /Contents {c2_id} 0 R >>".encode())
    # 6: Pages
    pages_id = sp.add(f"<< /Type /Pages /Kids [{page1_id} 0 R {page2_id} 0 R] /Count 2 >>".encode())
    # 7: Outlines root
    # 8: Item 1, 9: Item 2, 10: Item 2.1
    outlines_id = 7
    item1_id = 8
    item2_id = 9
    item21_id = 10
    sp.add(f"<< /Type /Outlines /First {item1_id} 0 R /Last {item2_id} 0 R /Count 3 >>".encode())
    sp.add(f"<< /Title (Architecture Overview) /Parent {outlines_id} 0 R /Next {item2_id} 0 R /Dest [{page1_id} 0 R /XYZ null null null] >>".encode())
    sp.add(f"<< /Title (Systems Engineering) /Parent {outlines_id} 0 R /Prev {item1_id} 0 R /First {item21_id} 0 R /Last {item21_id} 0 R /Count 1 /Dest [{page2_id} 0 R /XYZ null null null] >>".encode())
    sp.add(f"<< /Title (Performance & Bounded Caches) /Parent {item2_id} 0 R /Dest [{page2_id} 0 R /XYZ null null null] >>".encode())
    # 11: Catalog
    cat_id = sp.add(f"<< /Type /Catalog /Pages {pages_id} 0 R /Outlines {outlines_id} 0 R >>".encode())

    with open(os.path.join(fixtures_dir, "bookmarked_searchable.pdf"), "wb") as f:
        f.write(sp.write(cat_id))

    # 7. Large 20-Page Document for Stress Testing
    sp20 = SimplePdf()
    f_id = sp20.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>")
    page_ids = []
    # Reserve page IDs from 2 to 21
    # We will generate contents first then pages
    for p in range(1, 21):
        txt = f"BT /F1 16 Tf 72 700 Td (PDFinity Large Document Stress Test - Page {p} of 20) Tj /F1 11 Tf 0 -30 Td (This page verifies lazy loading, high-speed scrolling, and memory bound enforcement.) Tj ET".encode()
        cid = sp20.add(f"<< /Length {len(txt)} >>\r\nstream\r\n".encode() + txt + b"\r\nendstream")
        pid = sp20.add(b"") # placeholder
        page_ids.append((pid, cid))

    # Fill in actual page objects
    pages_root_id = len(sp20.objs) + 1
    for pid, cid in page_ids:
        sp20.objs[pid - 1] = f"<< /Type /Page /Parent {pages_root_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {f_id} 0 R >> >> /Contents {cid} 0 R >>".encode()

    kids_str = " ".join([f"{pid} 0 R" for pid, _ in page_ids])
    proot_id = sp20.add(f"<< /Type /Pages /Kids [{kids_str}] /Count 20 >>".encode())
    cat20_id = sp20.add(f"<< /Type /Catalog /Pages {proot_id} 0 R >>".encode())

    with open(os.path.join(fixtures_dir, "large_document_20p.pdf"), "wb") as f:
        f.write(sp20.write(cat20_id))

    # 8. Interactive AcroForm Sample PDF
    sp_form = SimplePdf()
    form_font = sp_form.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>") # 1
    form_content_txt = b"BT /F1 16 Tf 50 730 Td (PDFinity Interactive AcroForm Test Document) Tj /F1 10 Tf 0 -25 Td (Fill text, toggle checkbox, and pick dropdown options below:) Tj ET"
    form_cid = sp_form.add(f"<< /Length {len(form_content_txt)} >>\r\nstream\r\n".encode() + form_content_txt + b"\r\nendstream") # 2

    page_id = 3
    field_text_id = 4
    field_check_id = 5
    field_radio_id = 6
    field_combo_id = 7
    acroform_dict_id = 8
    pages_form_id = 9
    cat_form_id = 10

    # Page 1 (id 3)
    sp_form.add(f"<< /Type /Page /Parent {pages_form_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {form_font} 0 R >> >> /Contents {form_cid} 0 R /Annots [{field_text_id} 0 R {field_check_id} 0 R {field_radio_id} 0 R {field_combo_id} 0 R] >>".encode())

    # Text Field: FullName (id 4)
    sp_form.add(f"<< /Type /Annot /Subtype /Widget /FT /Tx /T (FullName) /TU (Enter your full name) /V (Jane Doe) /DV (Default Name) /Rect [50 650 250 675] /P {page_id} 0 R /F 4 >>".encode())

    # CheckBox: SubscribeNewsletter (id 5)
    sp_form.add(f"<< /Type /Annot /Subtype /Widget /FT /Btn /T (SubscribeNewsletter) /TU (Subscribe to newsletter updates) /V /Yes /AS /Yes /Rect [50 600 70 620] /P {page_id} 0 R /F 4 >>".encode())

    # RadioButton: BillingCycle (id 6)
    sp_form.add(f"<< /Type /Annot /Subtype /Widget /FT /Btn /Ff 32768 /T (BillingCycle) /TU (Select billing cycle) /V /Annual /AS /Annual /Rect [50 550 70 570] /P {page_id} 0 R /F 4 >>".encode())

    # Dropdown / ComboBox: Country (id 7)
    sp_form.add(f"<< /Type /Annot /Subtype /Widget /FT /Ch /Ff 131072 /T (Country) /TU (Select your resident country) /V (Canada) /DV (USA) /Opt [(USA) (Canada) (United Kingdom) (Germany) (Japan)] /Rect [50 490 220 515] /P {page_id} 0 R /F 4 >>".encode())

    # AcroForm Root (id 8)
    sp_form.add(f"<< /Fields [{field_text_id} 0 R {field_check_id} 0 R {field_radio_id} 0 R {field_combo_id} 0 R] /NeedAppearances true /DR << /Font << /F1 {form_font} 0 R >> >> >>".encode())

    # Pages root (id 9)
    sp_form.add(f"<< /Type /Pages /Kids [{page_id} 0 R] /Count 1 >>".encode())

    # Catalog (id 10)
    sp_form.add(f"<< /Type /Catalog /Pages {pages_form_id} 0 R /AcroForm {acroform_dict_id} 0 R >>".encode())

    with open(os.path.join(fixtures_dir, "acroform_sample.pdf"), "wb") as f:
        f.write(sp_form.write(cat_form_id))

    # 9. XFA Form Sample PDF (Discloses unsupported/partial behavior)
    sp_xfa = SimplePdf()
    xfa_font = sp_xfa.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>") # 1
    xfa_txt = b"BT /F1 16 Tf 50 730 Td (Adobe LiveCycle XFA Dynamic Form) Tj /F1 10 Tf 0 -25 Td (This document contains dynamic XML Forms Architecture data.) Tj ET"
    xfa_cid = sp_xfa.add(f"<< /Length {len(xfa_txt)} >>\r\nstream\r\n".encode() + xfa_txt + b"\r\nendstream") # 2

    xfa_xml_data = b"<xdp:xdp xmlns:xdp=\"http://ns.adobe.com/xdp/\"><template xmlns=\"http://www.xfa.org/schema/xfa-template/3.3/\"><subform name=\"form1\"/></template></xdp:xdp>"
    xfa_stream_id = sp_xfa.add(f"<< /Length {len(xfa_xml_data)} >>\r\nstream\r\n".encode() + xfa_xml_data + b"\r\nendstream") # 3

    xfa_page_id = 4
    xfa_acro_id = 5
    xfa_pages_id = 6
    xfa_cat_id = 7

    sp_xfa.add(f"<< /Type /Page /Parent {xfa_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {xfa_font} 0 R >> >> /Contents {xfa_cid} 0 R >>".encode()) # 4
    sp_xfa.add(f"<< /Fields [] /XFA [(xdp:xdp) {xfa_stream_id} 0 R] >>".encode()) # 5
    sp_xfa.add(f"<< /Type /Pages /Kids [{xfa_page_id} 0 R] /Count 1 >>".encode()) # 6
    sp_xfa.add(f"<< /Type /Catalog /Pages {xfa_pages_id} 0 R /AcroForm {xfa_acro_id} 0 R >>".encode()) # 7

    with open(os.path.join(fixtures_dir, "xfa_sample.pdf"), "wb") as f:
        f.write(sp_xfa.write(xfa_cat_id))

    # 10. Embedded Subset Font PDF (ABCDEF+Helvetica)
    sp_subset = SimplePdf()
    fd_id = sp_subset.add(b"<< /Type /FontDescriptor /FontName /ABCDEF+Helvetica /Flags 32 >>") # 1
    font_sub_id = sp_subset.add(f"<< /Type /Font /Subtype /TrueType /BaseFont /ABCDEF+Helvetica /FontDescriptor {fd_id} 0 R >>".encode()) # 2
    sub_txt = b"BT /F1 14 Tf 50 700 Td (This document uses an embedded subset font ABCDEF+Helvetica.) Tj ET"
    sub_cid = sp_subset.add(f"<< /Length {len(sub_txt)} >>\r\nstream\r\n".encode() + sub_txt + b"\r\nendstream") # 3
    sub_page_id = 4
    sub_pages_id = 5
    sub_cat_id = 6
    sp_subset.add(f"<< /Type /Page /Parent {sub_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {font_sub_id} 0 R >> >> /Contents {sub_cid} 0 R >>".encode()) # 4
    sp_subset.add(f"<< /Type /Pages /Kids [{sub_page_id} 0 R] /Count 1 >>".encode()) # 5
    sp_subset.add(f"<< /Type /Catalog /Pages {sub_pages_id} 0 R >>".encode()) # 6
    with open(os.path.join(fixtures_dir, "subset_font.pdf"), "wb") as f:
        f.write(sp_subset.write(sub_cat_id))

    # 11. Unicode Text PDF
    sp_uni = SimplePdf()
    uni_font = sp_uni.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>") # 1
    uni_txt = b"BT /F1 14 Tf 50 720 Td (PDFinity Unicode Text Fixture) Tj /F1 11 Tf 0 -30 Td (Standard ASCII: Hello World) Tj 0 -25 Td (Extended Symbols: Copyright and Em-Dash included.) Tj ET"
    uni_cid = sp_uni.add(f"<< /Length {len(uni_txt)} >>\r\nstream\r\n".encode() + uni_txt + b"\r\nendstream") # 2
    uni_page_id = 3
    uni_pages_id = 4
    uni_cat_id = 5
    sp_uni.add(f"<< /Type /Page /Parent {uni_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {uni_font} 0 R >> >> /Contents {uni_cid} 0 R >>".encode()) # 3
    sp_uni.add(f"<< /Type /Pages /Kids [{uni_page_id} 0 R] /Count 1 >>".encode()) # 4
    sp_uni.add(f"<< /Type /Catalog /Pages {uni_pages_id} 0 R >>".encode()) # 5
    with open(os.path.join(fixtures_dir, "unicode_text.pdf"), "wb") as f:
        f.write(sp_uni.write(uni_cat_id))

    # 12. Rotated Text PDF (45 deg and 90 deg)
    sp_rot = SimplePdf()
    rot_font = sp_rot.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>") # 1
    rot_txt = (b"BT /F1 14 Tf 0.7071 0.7071 -0.7071 0.7071 100 300 Tm (Rotated 45 Degrees) Tj "
               b"0 1 -1 0 100 500 Tm (Rotated 90 Degrees) Tj ET")
    rot_cid = sp_rot.add(f"<< /Length {len(rot_txt)} >>\r\nstream\r\n".encode() + rot_txt + b"\r\nendstream") # 2
    rot_page_id = 3
    rot_pages_id = 4
    rot_cat_id = 5
    sp_rot.add(f"<< /Type /Page /Parent {rot_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {rot_font} 0 R >> >> /Contents {rot_cid} 0 R >>".encode()) # 3
    sp_rot.add(f"<< /Type /Pages /Kids [{rot_page_id} 0 R] /Count 1 >>".encode()) # 4
    sp_rot.add(f"<< /Type /Catalog /Pages {rot_pages_id} 0 R >>".encode()) # 5
    with open(os.path.join(fixtures_dir, "rotated_text.pdf"), "wb") as f:
        f.write(sp_rot.write(rot_cat_id))

    # 13. Two-Column Layout PDF (for Collision & Overflow Testing)
    sp_col = SimplePdf()
    col_font = sp_col.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>") # 1
    col_txt = (b"BT /F1 12 Tf 50 700 Td (Column 1 Header Text) Tj 0 -20 Td (Left column body paragraph here.) Tj ET "
               b"BT /F1 12 Tf 320 700 Td (Column 2 Header Text) Tj 0 -20 Td (Right column adjacent text block.) Tj ET")
    col_cid = sp_col.add(f"<< /Length {len(col_txt)} >>\r\nstream\r\n".encode() + col_txt + b"\r\nendstream") # 2
    col_page_id = 3
    col_pages_id = 4
    col_cat_id = 5
    sp_col.add(f"<< /Type /Page /Parent {col_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {col_font} 0 R >> >> /Contents {col_cid} 0 R >>".encode()) # 3
    sp_col.add(f"<< /Type /Pages /Kids [{col_page_id} 0 R] /Count 1 >>".encode()) # 4
    sp_col.add(f"<< /Type /Catalog /Pages {col_pages_id} 0 R >>".encode()) # 5
    with open(os.path.join(fixtures_dir, "two_column_text.pdf"), "wb") as f:
        f.write(sp_col.write(col_cat_id))

    # 14. Vector Art & Path Objects PDF
    sp_vec = SimplePdf()
    vec_font = sp_vec.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>") # 1
    vec_content = (b"BT /F1 14 Tf 50 720 Td (Vector Art and Geometric Path Objects) Tj ET "
                   b"1 0 0 rg 0 0 1 RG 2 w 50 550 150 100 re B " # filled & stroked rectangle
                   b"0 1 0 rg 1 w 250 550 m 350 650 l 400 550 l h B " # filled triangle
                   b"0 0 0 RG 3 w 50 450 m 150 520 250 400 350 480 c S") # bezier curve stroke
    vec_cid = sp_vec.add(f"<< /Length {len(vec_content)} >>\r\nstream\r\n".encode() + vec_content + b"\r\nendstream") # 2
    vec_page_id = 3
    vec_pages_id = 4
    vec_cat_id = 5
    sp_vec.add(f"<< /Type /Page /Parent {vec_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {vec_font} 0 R >> >> /Contents {vec_cid} 0 R >>".encode()) # 3
    sp_vec.add(f"<< /Type /Pages /Kids [{vec_page_id} 0 R] /Count 1 >>".encode()) # 4
    sp_vec.add(f"<< /Type /Catalog /Pages {vec_pages_id} 0 R >>".encode()) # 5
    with open(os.path.join(fixtures_dir, "vector_art.pdf"), "wb") as f:
        f.write(sp_vec.write(vec_cat_id))

    # 15. Scanned Page with Raster Image Object PDF
    sp_scan = SimplePdf()
    pixels = []
    for r in range(16):
        for c in range(16):
            val = 245 if ((r + c) % 2 == 0) else 50
            pixels.extend([val, val, val])
    img_bytes = bytes(pixels)
    img_id = sp_scan.add(f"<< /Type /XObject /Subtype /Image /Width 16 /Height 16 /ColorSpace /DeviceRGB /BitsPerComponent 8 /Length {len(img_bytes)} >>\r\nstream\r\n".encode() + img_bytes + b"\r\nendstream") # 1
    scan_txt = f"q 500 0 0 700 50 50 cm /Im1 Do Q".encode()
    scan_cid = sp_scan.add(f"<< /Length {len(scan_txt)} >>\r\nstream\r\n".encode() + scan_txt + b"\r\nendstream") # 2
    scan_page_id = 3
    scan_pages_id = 4
    scan_cat_id = 5
    sp_scan.add(f"<< /Type /Page /Parent {scan_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /XObject << /Im1 {img_id} 0 R >> >> /Contents {scan_cid} 0 R >>".encode()) # 3
    sp_scan.add(f"<< /Type /Pages /Kids [{scan_page_id} 0 R] /Count 1 >>".encode()) # 4
    sp_scan.add(f"<< /Type /Catalog /Pages {scan_pages_id} 0 R >>".encode()) # 5
    with open(os.path.join(fixtures_dir, "scanned_page.pdf"), "wb") as f:
        f.write(sp_scan.write(scan_cat_id))

    # 16. Test PKCS#12 Certificate & Key Fixtures for Cryptographic Signing
    openssl_path = shutil.which("openssl")
    if not openssl_path:
        for candidate in [
            r"C:\Program Files\Git\mingw64\bin\openssl.exe",
            r"C:\Program Files\OpenSSL-Win64\bin\openssl.exe"
        ]:
            if os.path.exists(candidate):
                openssl_path = candidate
                break

    if openssl_path:
        pfx_path = os.path.join(fixtures_dir, "test_signer.pfx")
        crt_path = os.path.join(fixtures_dir, "test_signer.crt")
        key_tmp = os.path.join(fixtures_dir, "temp_key.pem")
        cert_tmp = os.path.join(fixtures_dir, "temp_cert.pem")

        # Generate self-signed certificate and private key
        gen_cmd = [
            openssl_path, "req", "-x509", "-newkey", "rsa:2048",
            "-keyout", key_tmp, "-out", cert_tmp,
            "-days", "3650", "-nodes",
            "-subj", "/CN=PDFinity Test Signer/O=PDFinity QA/C=US"
        ]
        subprocess.run(gen_cmd, check=True, capture_output=True)

        # Export to PKCS#12 / PFX
        pfx_cmd = [
            openssl_path, "pkcs12", "-export",
            "-out", pfx_path,
            "-inkey", key_tmp,
            "-in", cert_tmp,
            "-passout", "pass:testpass123",
            "-name", "PDFinity Test Signer"
        ]
        subprocess.run(pfx_cmd, check=True, capture_output=True)

        # Save public cert
        shutil.copyfile(cert_tmp, crt_path)

        # Cleanup temporary PEM files
        if os.path.exists(key_tmp): os.remove(key_tmp)
        if os.path.exists(cert_tmp): os.remove(cert_tmp)
        print("Generated PKCS#12 test certificate fixture: test_signer.pfx")

    # 17. Redaction & Sanitization Adversarial Test Corpus
    sp_adv = SimplePdf()
    adv_font = sp_adv.add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>") # 1
    adv_txt = (
        b"BT /F1 12 Tf 50 720 Td (TOP_SECRET_CODENAME_ALPHA99) Tj\r\n"
        b"/F1 12 Tf 0 -40 Td (Employee SSN: 999-88-7777 - Classified Personnel) Tj\r\n"
        b"/F1 12 Tf 0 -40 Td (Public Notice: This document contains unclassified general information.) Tj ET\r\n"
    )
    adv_cid = sp_adv.add(f"<< /Length {len(adv_txt)} >>\r\nstream\r\n".encode() + adv_txt + b"\r\nendstream") # 2

    # 16x16 embedded image on page 1
    pixels = []
    for r in range(16):
        for c in range(16):
            pixels.extend([220, 20, 20])
    img_bytes = bytes(pixels)
    adv_img_id = sp_adv.add(f"<< /Type /XObject /Subtype /Image /Width 16 /Height 16 /ColorSpace /DeviceRGB /BitsPerComponent 8 /Length {len(img_bytes)} >>\r\nstream\r\n".encode() + img_bytes + b"\r\nendstream") # 3

    # Form XObject on Page 2 for testing unsupported content detection
    form_stream = b"q /F1 10 Tf 10 10 Td (Nested FormXObject Protected Text) Tj Q"
    form_xobj_id = sp_adv.add(f"<< /Type /XObject /Subtype /Form /BBox [0 0 200 100] /Resources << /Font << /F1 {adv_font} 0 R >> >> /Length {len(form_stream)} >>\r\nstream\r\n".encode() + form_stream + b"\r\nendstream") # 4

    p2_cid = sp_adv.add(b"<< /Length 30 >>\r\nstream\r\nq 1 0 0 1 50 600 cm /Fm1 Do Q\r\nendstream") # 5

    adv_page1_id = 6
    adv_page2_id = 7
    adv_pages_id = 8
    adv_info_id = 9
    adv_cat_id = 10

    # Page 1: Sensitive text and image
    sp_adv.add(f"<< /Type /Page /Parent {adv_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {adv_font} 0 R >> /XObject << /Im1 {adv_img_id} 0 R >> >> /Contents {adv_cid} 0 R >>".encode()) # 6
    # Page 2: Unsupported FormXObject
    sp_adv.add(f"<< /Type /Page /Parent {adv_pages_id} 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 {adv_font} 0 R >> /XObject << /Fm1 {form_xobj_id} 0 R >> >> /Contents {p2_cid} 0 R >>".encode()) # 7
    # Pages Root
    sp_adv.add(f"<< /Type /Pages /Kids [{adv_page1_id} 0 R {adv_page2_id} 0 R] /Count 2 >>".encode()) # 8
    # Info metadata
    sp_adv.add(b"<< /Title (Classified Operations Manual) /Author (Agent Zero) /Subject (Top Secret Eyes Only) /Keywords (classified, ssn, confidential) >>") # 9
    # Catalog
    sp_adv.add(f"<< /Type /Catalog /Pages {adv_pages_id} 0 R >>".encode()) # 10

    with open(os.path.join(fixtures_dir, "redaction_adversarial.pdf"), "wb") as f:
        f.write(sp_adv.write(adv_cat_id))
    print("Generated adversarial redaction fixture: redaction_adversarial.pdf")

    print("All test fixtures created successfully in:", fixtures_dir)

if __name__ == "__main__":
    create_fixtures()
