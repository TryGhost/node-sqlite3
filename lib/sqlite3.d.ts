// Type definitions for sqlite3
// Project: http://github.com/tryghost/node-sqlite3

/// <reference types="node" />

import events = require("events");

export const OPEN_READONLY: number;
export const OPEN_READWRITE: number;
export const OPEN_CREATE: number;
export const OPEN_FULLMUTEX: number;
export const OPEN_SHAREDCACHE: number;
export const OPEN_PRIVATECACHE: number;
export const OPEN_URI: number;

export const VERSION: string;
export const SOURCE_ID: string;
export const VERSION_NUMBER: number;

export const OK: number;
export const ERROR: number;
export const INTERNAL: number;
export const PERM: number;
export const ABORT: number;
export const BUSY: number;
export const LOCKED: number;
export const NOMEM: number;
export const READONLY: number;
export const INTERRUPT: number
export const IOERR: number;
export const CORRUPT: number
export const NOTFOUND: number;
export const FULL: number;
export const CANTOPEN: number;
export const PROTOCOL: number;
export const EMPTY: number;
export const SCHEMA: number;
export const TOOBIG: number
export const CONSTRAINT: number
export const MISMATCH: number;
export const MISUSE: number;
export const NOLFS: number;
export const AUTH: number
export const FORMAT: number;
export const RANGE: number
export const NOTADB: number;

export const LIMIT_LENGTH: number;
export const LIMIT_SQL_LENGTH: number;
export const LIMIT_COLUMN: number;
export const LIMIT_EXPR_DEPTH: number;
export const LIMIT_COMPOUND_SELECT: number;
export const LIMIT_VDBE_OP: number;
export const LIMIT_FUNCTION_ARG: number;
export const LIMIT_ATTACHED: number;
export const LIMIT_LIKE_PATTERN_LENGTH: number;
export const LIMIT_VARIABLE_NUMBER: number;
export const LIMIT_TRIGGER_DEPTH: number;
export const LIMIT_WORKER_THREADS: number;

export const cached: {
    Database(filename: string, callback?: (this: Database, err: Error | null) => void): Database;
    Database(filename: string, mode?: number, callback?: (this: Database, err: Error | null) => void): Database;
};

export interface RunResult extends Statement {
    lastID: number;
    changes: number;
}

export class Statement extends events.EventEmitter {
    bind(callback?: (err: Error | null) => void): this;
    bind(...params: any[]): this;

    reset(callback?: (err: null) => void): this;

    finalize(callback?: (err: Error) => void): Database;

    run(callback?: (err: Error | null) => void): this;
    run(params: any, callback?: (this: RunResult, err: Error | null) => void): this;
    run(...params: any[]): this;

    get<T>(callback?: (err: Error | null, row?: T) => void): this;
    get<T>(params: any, callback?: (this: RunResult, err: Error | null, row?: T) => void): this;
    get(...params: any[]): this;

    all<T>(callback?: (err: Error | null, rows: T[]) => void): this;
    all<T>(params: any, callback?: (this: RunResult, err: Error | null, rows: T[]) => void): this;
    all(...params: any[]): this;

    each<T>(callback?: (err: Error | null, row: T) => void, complete?: (err: Error | null, count: number) => void): this;
    each<T>(params: any, callback?: (this: RunResult, err: Error | null, row: T) => void, complete?: (err: Error | null, count: number) => void): this;
    each(...params: any[]): this;
}

export class Database extends events.EventEmitter {
    constructor(filename: string, callback?: (err: Error | null) => void);
    constructor(filename: string, mode?: number, callback?: (err: Error | null) => void);

    close(callback?: (err: Error | null) => void): void;

    run(sql: string, callback?: (this: RunResult, err: Error | null) => void): this;
    run(sql: string, params: any, callback?: (this: RunResult, err: Error | null) => void): this;
    run(sql: string, ...params: any[]): this;

    get<T>(sql: string, callback?: (this: Statement, err: Error | null, row: T) => void): this;
    get<T>(sql: string, params: any, callback?: (this: Statement, err: Error | null, row: T) => void): this;
    get(sql: string, ...params: any[]): this;

    all<T>(sql: string, callback?: (this: Statement, err: Error | null, rows: T[]) => void): this;
    all<T>(sql: string, params: any, callback?: (this: Statement, err: Error | null, rows: T[]) => void): this;
    all(sql: string, ...params: any[]): this;

    each<T>(sql: string, callback?: (this: Statement, err: Error | null, row: T) => void, complete?: (err: Error | null, count: number) => void): this;
    each<T>(sql: string, params: any, callback?: (this: Statement, err: Error | null, row: T) => void, complete?: (err: Error | null, count: number) => void): this;
    each(sql: string, ...params: any[]): this;

    exec(sql: string, callback?: (this: Statement, err: Error | null) => void): this;

    prepare(sql: string, callback?: (this: Statement, err: Error | null) => void): Statement;
    prepare(sql: string, params: any, callback?: (this: Statement, err: Error | null) => void): Statement;
    prepare(sql: string, ...params: any[]): Statement;

    serialize(callback?: () => void): void;
    parallelize(callback?: () => void): void;

    on(event: "trace", listener: (sql: string) => void): this;
    on(event: "profile", listener: (sql: string, time: number) => void): this;
    on(event: "change", listener: (type: string, database: string, table: string, rowid: number) => void): this;
    on(event: "error", listener: (err: Error) => void): this;
    on(event: "open" | "close", listener: () => void): this;
    on(event: string, listener: (...args: any[]) => void): this;

    configure(option: "busyTimeout", value: number): void;
    configure(option: "limit", id: number, value: number): void;

    loadExtension(filename: string, callback?: (err: Error | null) => void): this;

    wait(callback?: (param: null) => void): this;

    interrupt(): void;

    
    backup(path:string, callback?: ()=>void): Backup
    backup(filename:string, destDbName:string, sourceDbName:string, filenameIsDest:boolean, callback?: ()=>void): Backup
}

/**
 *
 * A class for managing an sqlite3_backup object.  For consistency
 * with other node-sqlite3 classes, it maintains an internal queue
 * of calls.
 *
 * Intended usage from node:
 *
 *   var db = new sqlite3.Database('live.db');
 *   var backup = db.backup('backup.db');
 *   ...
 *   // in event loop, move backup forward when we have time.
 *   if (backup.idle) { backup.step(NPAGES); }
 *   if (backup.completed) { ... success ... }
 *   if (backup.failed)    { ... sadness ... }
 *   // do other work in event loop - fine to modify live.db
 *   ...
 *
 * Here is how sqlite's backup api is exposed:
 *
 *   - `sqlite3_backup_init`: This is implemented as
 *     `db.backup(filename, [callback])` or
 *     `db.backup(filename, destDbName, sourceDbName, filenameIsDest, [callback])`.
 *   - `sqlite3_backup_step`: `backup.step(pages, [callback])`.
 *   - `sqlite3_backup_finish`: `backup.finish([callback])`.
 *   - `sqlite3_backup_remaining`: `backup.remaining`.
 *   - `sqlite3_backup_pagecount`: `backup.pageCount`.
 *
 * There are the following read-only properties:
 *
 *   - `backup.completed` is set to `true` when the backup
 *     succeeeds.
 *   - `backup.failed` is set to `true` when the backup
 *     has a fatal error.
 *   - `backup.message` is set to the error string 
 *     the backup has a fatal error.
 *   - `backup.idle` is set to `true` when no operation
 *     is currently in progress or queued for the backup.
 *   - `backup.remaining` is an integer with the remaining
 *     number of pages after the last call to `backup.step`
 *     (-1 if `step` not yet called).
 *   - `backup.pageCount` is an integer with the total number
 *     of pages measured during the last call to `backup.step`
 *     (-1 if `step` not yet called).
 *
 * There is the following writable property:
 *
 *   - `backup.retryErrors`: an array of sqlite3 error codes
 *     that are treated as non-fatal - meaning, if they occur,
 *     backup.failed is not set, and the backup may continue.
 *     By default, this is `[sqlite3.BUSY, sqlite3.LOCKED]`.
 *
 * The `db.backup(filename, [callback])` shorthand is sufficient
 * for making a backup of a database opened by node-sqlite3.  If
 * using attached or temporary databases, or moving data in the
 * opposite direction, the more complete (but daunting)
 * `db.backup(filename, destDbName, sourceDbName, filenameIsDest, [callback])`
 * signature is provided.
 *
 * A backup will finish automatically when it succeeds or a fatal
 * error occurs, meaning it is not necessary to call `db.finish()`.
 * By default, SQLITE_LOCKED and SQLITE_BUSY errors are not
 * treated as failures, and the backup will continue if they
 * occur.  The set of errors that are tolerated can be controlled
 * by setting `backup.retryErrors`. To disable automatic
 * finishing and stick strictly to sqlite's raw api, set
 * `backup.retryErrors` to `[]`.  In that case, it is necessary
 * to call `backup.finish()`.
 *
 * In the same way as node-sqlite3 databases and statements,
 * backup methods can be called safely without callbacks, due
 * to an internal call queue.  So for example this naive code
 * will correctly back up a db, if there are no errors:
 *
 *   var backup = db.backup('backup.db');
 *   backup.step(-1);
 *   backup.finish();
 *
 */
export class Backup extends events.EventEmitter {
    /**
     * `true` when the backup is idle and ready for `step()` to 
     * be called, `false` when busy.
     */
    readonly idle: boolean

    /**
     * `true` when the backup has completed, `false` otherwise.
     */
    readonly completed: boolean

    /**
     * `true` when the backup has failed, `false` otherwise. `Backup.message`
     * contains the error message.
     */
    readonly failed: boolean

    /**
     * Message failure string from sqlite3_errstr() if `Backup.failed` is `true`
     */
    readonly message: boolean

    /**
     * The number of remaining pages after the last call to `step()`, 
     * or `-1` if `step()` has never been called.
     */
    readonly remaining: number

    /**
     * The total number of pages measured during the last call to `step()`, 
     * or `-1` if `step()` has never been called.
     */
    readonly pageCount: number


    /**
     * An array of sqlite3 error codes that are treated as non-fatal - 
     * meaning, if they occur, `Backup.failed` is not set, and the backup 
     * may continue. By default, this is `[sqlite3.BUSY, sqlite3.LOCKED]`.
     */
    retryErrors: number[]

    /**
     * Asynchronously finalize the backup (required).
     * 
     * @param callback Called when the backup is finalized.
     */
    finish(callback?: ()=>void): void

    /**
     * Asynchronously perform an incremental segment of the backup.
     * 
     * Example:
     * 
     * ```
     * backup.step(5)
     * ```
     * 
     * @param nPages Number of pages to process (5 recommended).
     * @param callback Called when the step is completed.
     */
    step(nPages: number,callback?: ()=>void): void
}

export function verbose(): sqlite3;

export interface sqlite3 {
    OPEN_READONLY: number;
    OPEN_READWRITE: number;
    OPEN_CREATE: number;
    OPEN_FULLMUTEX: number;
    OPEN_SHAREDCACHE: number;
    OPEN_PRIVATECACHE: number;
    OPEN_URI: number;

    VERSION: string;
    SOURCE_ID: string;
    VERSION_NUMBER: number;

    OK: number;
    ERROR: number;
    INTERNAL: number;
    PERM: number;
    ABORT: number;
    BUSY: number;
    LOCKED: number;
    NOMEM: number;
    READONLY: number;
    INTERRUPT: number
    IOERR: number;
    CORRUPT: number
    NOTFOUND: number;
    FULL: number;
    CANTOPEN: number;
    PROTOCOL: number;
    EMPTY: number;
    SCHEMA: number;
    TOOBIG: number
    CONSTRAINT: number
    MISMATCH: number;
    MISUSE: number;
    NOLFS: number;
    AUTH: number
    FORMAT: number;
    RANGE: number
    NOTADB: number;

    LIMIT_LENGTH: number;
    LIMIT_SQL_LENGTH: number;
    LIMIT_COLUMN: number;
    LIMIT_EXPR_DEPTH: number;
    LIMIT_COMPOUND_SELECT: number;
    LIMIT_VDBE_OP: number;
    LIMIT_FUNCTION_ARG: number;
    LIMIT_ATTACHED: number;
    LIMIT_LIKE_PATTERN_LENGTH: number;
    LIMIT_VARIABLE_NUMBER: number;
    LIMIT_TRIGGER_DEPTH: number;
    LIMIT_WORKER_THREADS: number;

    cached: typeof cached;
    RunResult: RunResult;
    Statement: typeof Statement;
    Database: typeof Database;
    verbose(): this;
}