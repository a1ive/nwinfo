#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import re
import datetime

# PyMuPDF is imported as fitz
try:
    import fitz
except ImportError:
    print("FATAL: PyMuPDF library not found.")
    print("--> Please install it by running: pip install pymupdf")
    sys.exit(1)

START_PAGE_INDEX = 5
DATE_SEARCH_PAGE_INDEX = 4

def extract_document_name(doc):
    """Extracts the document identifier (e.g., JEP106BM) from the PDF."""
    print("[INFO] Extracting document name...")
    # Attempt to find in metadata first
    meta_title = doc.metadata.get('title', '')
    match = re.search(r'(JEP106[A-Z]{2})', meta_title, re.IGNORECASE)
    if match:
        name = match.group(1).upper()
        print("  |-- Found in metadata: '{}'".format(name))
        return name

    # Fallback to scanning the cover page
    try:
        print("  |-- Not in metadata, scanning cover page...")
        cover_page_text = doc.load_page(0).get_text("text")
        match = re.search(r'(JEP106[A-Z]{2})', cover_page_text, re.IGNORECASE)
        if match:
            name = match.group(1).upper()
            print("  |-- Found on cover page: '{}'".format(name))
            return name
    except Exception as e:
        print("  |-- [WARN] Error scanning cover page: {}".format(e))

    print("  |-- [WARN] Could not find name. Using default 'JEP106BM'.")
    return "JEP106BM"

def extract_document_date(doc):
    """Scans a specific page of the PDF to find the document's effective date."""
    print("[INFO] Extracting document date from page {}...".format(DATE_SEARCH_PAGE_INDEX + 1))
    try:
        page = doc.load_page(DATE_SEARCH_PAGE_INDEX)
        text = page.get_text("text")

        match = re.search(r'The present list is complete as of\s+(.*?)\.', text, re.IGNORECASE)
        if match:
            date_str = match.group(1).strip()
            dt_obj = datetime.datetime.strptime(date_str, "%B %d, %Y")
            formatted_date = dt_obj.strftime("%Y.%m.%d")
            print("  |-- Found and parsed date: '{}'".format(formatted_date))
            return formatted_date
    except Exception as e:
        print("  |-- [WARN] Failed to parse date: {}".format(e))

    print("  |-- [WARN] Could not extract date. Using current date as fallback.")
    return datetime.date.today().strftime("%Y.%m.%d")

def clean_manufacturer_name(raw_name):
    """
    Cleans the raw manufacturer name by stripping trailing table data and
    normalizing internal whitespace.
    """
    # " 1 1 0 0 0 1 1 1 C7"
    cleaned_name = re.sub(r'(\s+[01])+(\s+[0-9A-Fa-f]{2})?$', '', raw_name)

    # Replace sequences of one or more whitespace characters with a single space.
    cleaned_name = re.sub(r'\s+', ' ', cleaned_name)

    # Handle non-ASCII punctuation
    replacements = {
        '\u2019': "'",  # Right Single Quotation Mark -> Apostrophe
        '\u2018': "'",  # Left Single Quotation Mark  -> Apostrophe
        '\u201d': '"',  # Right Double Quotation Mark -> Quotation Mark
        '\u201c': '"',  # Left Double Quotation Mark  -> Quotation Mark
        '\u2014': '-',  # Em Dash -> Hyphen
        '\u2013': '-',  # En Dash -> Hyphen
    }

    for old, new in replacements.items():
        cleaned_name = cleaned_name.replace(old, new)

    return cleaned_name.strip()

def parse_jep106_pdf(input_path, output_path):
    """
    Parses the JEP106 PDF file.
    """
    print("--- JEP106 Parser Started ---")
    print("[INFO] Input PDF: {}".format(input_path))
    print("[INFO] Output file: {}".format(output_path))

    try:
        doc = fitz.open(input_path)
        print("[OK] PDF file opened successfully ({} pages).".format(len(doc)))
    except Exception as e:
        print("FATAL: Failed to open or read the PDF file '{}'.".format(input_path))
        print("--> Error: {}".format(e))
        return

    output_lines = []
    output_lines.append('')

    # Header generation
    output_lines.append("# {}".format(extract_document_name(doc)))
    output_lines.append("# Version: {}".format(extract_document_date(doc)))
    print("[OK] File header generated.")

    current_bank = 0
    manufacturer_count = 0
    line_pattern = re.compile(r'^(\d{1,3})\s+(.*)')
    print("\n--- Starting Parsing ---")

    for page_num in range(START_PAGE_INDEX, len(doc)):
        print("\n[PAGE {}/{}]".format(page_num + 1, len(doc)))
        page = doc.load_page(page_num)
        text = page.get_text("text")

        # Check for the start of the appendix to stop parsing.
        if "Annex A (informative) Name Changes" in text:
            print("  [STOP] Detected start of Annex A. Terminating main content parsing.")
            break

        lines = text.split('\n')

        if page_num == START_PAGE_INDEX and current_bank == 0:
            current_bank = 1
            output_lines.append(str(current_bank))
            print("  -> Initialized to Bank {}.".format(current_bank))

        # Check for bank switch text before parsing lines to correctly associate all entries
        if "The following numbers are all in bank" in text:
            current_bank += 1
            output_lines.append(str(current_bank))
            print("  -> Detected switch to Bank {}.".format(current_bank))

        i = 0
        while i < len(lines):
            line = lines[i]
            line_stripped = line.strip()

            # Immediately prepare for the next iteration.
            i += 1

            if not line_stripped:
                continue

            match = line_pattern.match(line_stripped)
            if match:
                id_code, raw_name = match.groups()

                # Look ahead to the next line for a possible continuation.
                # A continuation line is a non-empty line that does NOT start with another ID.
                if i < len(lines):  # Check if a next line exists.
                    next_line_stripped = lines[i].strip()
                    # Use regex to check if the next line is a continuation.
                    if next_line_stripped and not line_pattern.match(next_line_stripped):
                        # It's a continuation. Append it to the raw name.
                        raw_name = f"{raw_name} {next_line_stripped}"
                        # We have consumed the next line, so advance the index again.
                        i += 1

                # Skip entries that are not actual manufacturers
                if "Continuation Code" in raw_name:
                    print("  [SKIP] 'Continuation Code' entry.")
                    continue

                final_name = clean_manufacturer_name(raw_name)
                output_lines.append("\t{} {}".format(id_code, final_name))
                manufacturer_count += 1

                print("  [OK]   ID: {:<4} Name: {}".format(id_code, final_name))

    print("\n--- Parsing Complete ---")
    print("[INFO] Total manufacturers found: {}".format(manufacturer_count))
    print("[INFO] Total banks processed: {}".format(current_bank))
    print("[INFO] Writing {} final lines to '{}'...".format(len(output_lines), output_path))

    try:
        with open(output_path, 'w', encoding='ascii', errors='strict') as f:
            f.write('\n'.join(output_lines))
            f.write('\n')
        print("\n--- SUCCESS ---")
        print("Generated file: '{}'".format(output_path))
    except Exception as e:
        print("\n--- FATAL ERROR ---")
        print("Failed to write to the output file '{}'.".format(output_path))
        print("--> Character Encoding Error or I/O issue.")
        print("--> Detailed Error: {}".format(e))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python {} <input_pdf_file> <output_ids_file>".format(sys.argv[0]))
        sys.exit(1)

    parse_jep106_pdf(sys.argv[1], sys.argv[2])
